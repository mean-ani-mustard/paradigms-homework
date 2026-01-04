#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>
#include <pthread.h>
#include <string.h>

#include "bank.h"
#include "branch.h"
#include "account.h"
#include "report.h"
#include "debug.h"

#define MAX_NUM_REPORTS 8
#define MAX_LOG_ENTRIES 1024

typedef struct TransferLog {
  AccountNumber accountNum;
  AccountAmount transferSize;
} TransferLog;

typedef struct WorkerLog {
  int stored;
  int total;
  TransferLog entries[MAX_LOG_ENTRIES];
} WorkerLog;

typedef struct Report {
  int numReports;

  struct {
    AccountAmount balance;
    int hasOverflowed;
    int numLogEntries;
    TransferLog transferLog[MAX_LOG_ENTRIES];
  } dailyData[MAX_NUM_REPORTS];

  AccountAmount reportingAmount;
  int numWorkers;

  pthread_mutex_t barLock;
  pthread_cond_t  barCond;
  int arrived;
  int generation;

  int currentDay;
  WorkerLog *wlogs;
} Report;

static int
TransferLogSortFunc(const void *p1, const void *p2)
{
  const TransferLog *l1 = (const TransferLog *)p1;
  const TransferLog *l2 = (const TransferLog *)p2;

  if (l1->accountNum < l2->accountNum) return -1;
  if (l1->accountNum > l2->accountNum) return 1;
  if (l1->transferSize < l2->transferSize) return -1;
  if (l1->transferSize > l2->transferSize) return 1;
  return 0;
}

static inline int
AtomicLoadDay(Report *r)
{
  return __atomic_load_n(&r->currentDay, __ATOMIC_ACQUIRE);
}

static inline void
AtomicStoreDay(Report *r, int v)
{
  __atomic_store_n(&r->currentDay, v, __ATOMIC_RELEASE);
}

static void
ClearDayLogs(Report *r, int day)
{
  if (day < 0 || day >= MAX_NUM_REPORTS) return;
  for (int w = 0; w < r->numWorkers; w++) {
    WorkerLog *wl = &r->wlogs[day * r->numWorkers + w];
    wl->stored = 0;
    wl->total = 0;
  }
}

static void
AggregateDay(Report *r, int day)
{
  int total = 0;
  for (int w = 0; w < r->numWorkers; w++) {
    WorkerLog *wl = &r->wlogs[day * r->numWorkers + w];
    total += wl->total;
  }

  r->dailyData[day].hasOverflowed = (total > MAX_LOG_ENTRIES);
  r->dailyData[day].numLogEntries = (total > MAX_LOG_ENTRIES) ? MAX_LOG_ENTRIES : total;

  int out = 0;
  for (int w = 0; w < r->numWorkers && out < MAX_LOG_ENTRIES; w++) {
    WorkerLog *wl = &r->wlogs[day * r->numWorkers + w];
    for (int i = 0; i < wl->stored && out < MAX_LOG_ENTRIES; i++) {
      r->dailyData[day].transferLog[out++] = wl->entries[i];
    }
  }
}

int
Report_Init(struct Bank *bank, AccountAmount reportAmount, int maxNumWorkers)
{
  bank->report = (Report *)malloc(sizeof(Report));
  if (bank->report == NULL) return -1;

  Report *r = bank->report;
  r->numReports = 0;

  for (int d = 0; d < MAX_NUM_REPORTS; d++) {
    r->dailyData[d].hasOverflowed = 0;
    r->dailyData[d].numLogEntries = 0;
  }

  r->reportingAmount = reportAmount;
  r->numWorkers = maxNumWorkers;

  pthread_mutex_init(&r->barLock, NULL);
  pthread_cond_init(&r->barCond, NULL);
  r->arrived = 0;
  r->generation = 0;

  AtomicStoreDay(r, 0);

  r->wlogs = (WorkerLog *)malloc(sizeof(WorkerLog) * MAX_NUM_REPORTS * r->numWorkers);
  if (r->wlogs == NULL) return -1;

  for (int d = 0; d < MAX_NUM_REPORTS; d++) ClearDayLogs(r, d);

  return 0;
}

int
Report_Transfer(struct Bank *bank, int workerNum, AccountNumber accountNum, AccountAmount amount)
{
  Report *r = bank->report;

  AccountAmount abs = (amount < 0) ? -amount : amount;
  Y;
  if (abs < r->reportingAmount) return 0;

  int day = AtomicLoadDay(r);
  if (day < 0 || day >= MAX_NUM_REPORTS) return 0;
  if (workerNum < 0 || workerNum >= r->numWorkers) return 0;

  WorkerLog *wl = &r->wlogs[day * r->numWorkers + workerNum];

  wl->total++;
  if (wl->stored < MAX_LOG_ENTRIES) {
    int i = wl->stored;
    wl->entries[i].accountNum = accountNum;
    wl->entries[i].transferSize = amount;
    wl->stored = i + 1;
  }

  return 0;
}

int
Report_DoReport(struct Bank *bank, int workerNum)
{
  Report *r = bank->report;
  assert(r);
  Y;

  pthread_mutex_lock(&r->barLock);
  int myGen = r->generation;

  r->arrived++;
  if (r->arrived < r->numWorkers) {
    while (myGen == r->generation)
      pthread_cond_wait(&r->barCond, &r->barLock);
    pthread_mutex_unlock(&r->barLock);
    return 0;
  }

  int day = AtomicLoadDay(r);
  if (day >= MAX_NUM_REPORTS) {
    r->arrived = 0;
    r->generation++;
    pthread_cond_broadcast(&r->barCond);
    pthread_mutex_unlock(&r->barLock);
    return -1;
  }

  int err = Bank_Balance(bank, &r->dailyData[day].balance);
  AggregateDay(r, day);

  r->numReports = day + 1;

  AtomicStoreDay(r, day + 1);
  if (day + 1 < MAX_NUM_REPORTS) ClearDayLogs(r, day + 1);

  r->arrived = 0;
  r->generation++;
  pthread_cond_broadcast(&r->barCond);
  pthread_mutex_unlock(&r->barLock);

  return err;
}

int
Report_Compare(struct Bank *bank1, struct Bank *bank2)
{
  int err = 0;

  Report *rpt1 = bank1->report;
  Report *rpt2 = bank2->report;

  if (rpt1->numReports != rpt2->numReports) {
    fprintf(stderr, "Bank num reports mismatch %d != %d\n",
            rpt1->numReports, rpt2->numReports);
    err = -1;
  }

  for (int r = 0; r < rpt1->numReports; r++) {
    if (rpt1->dailyData[r].balance != rpt2->dailyData[r].balance) {
      fprintf(stderr, "Report %d for banks mismatch %"PRId64" and %"PRId64"\n",
              r, rpt1->dailyData[r].balance, rpt2->dailyData[r].balance);
      err = -1;
    }
    if (rpt1->dailyData[r].numLogEntries != rpt2->dailyData[r].numLogEntries) {
      fprintf(stderr, "Report different number of log entries (%d and %d)\n",
              rpt1->dailyData[r].numLogEntries,
              rpt2->dailyData[r].numLogEntries);
      return -1;
    }
    if (!rpt1->dailyData[r].hasOverflowed) {
      int n = rpt1->dailyData[r].numLogEntries;
      qsort(rpt1->dailyData[r].transferLog, n, sizeof(TransferLog), TransferLogSortFunc);
      qsort(rpt2->dailyData[r].transferLog, n, sizeof(TransferLog), TransferLogSortFunc);

      for (int i = 0; i < n; i++) {
        if ((rpt1->dailyData[r].transferLog[i].accountNum != rpt2->dailyData[r].transferLog[i].accountNum) ||
            (rpt1->dailyData[r].transferLog[i].transferSize != rpt2->dailyData[r].transferLog[i].transferSize)) {
          fprintf(stderr, "Report transferLog %d difference at %d\n", r, i);
          err = -1;
        }
      }
    }
  }

  return err;
}
