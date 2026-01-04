#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>

#include "bank.h"
#include "branch.h"
#include "report.h"

Bank*
Bank_Init(int numBranches, int numAccounts, AccountAmount initalAmount,
          AccountAmount reportingAmount,
          int numWorkers)
{
  Bank *bank = malloc(sizeof(Bank));
  if (bank == NULL) return bank;

  Branch_Init(bank, numBranches, numAccounts, initalAmount);
  Report_Init(bank, reportingAmount, numWorkers);

  return bank;
}

int
Bank_Balance(Bank *bank, AccountAmount *balance)
{
  assert(bank->branches);

  for (unsigned int i = 0; i < bank->numberBranches; i++)
    pthread_rwlock_rdlock(&bank->branches[i].rwlock);

  AccountAmount total = 0;
  for (unsigned int i = 0; i < bank->numberBranches; i++)
    total += bank->branches[i].balance;

  for (int i = (int)bank->numberBranches - 1; i >= 0; i--)
    pthread_rwlock_unlock(&bank->branches[i].rwlock);

  *balance = total;
  return 0;
}

int
Bank_Validate(Bank *bank)
{
  assert(bank->branches);
  int err = 0;

  for (unsigned int branch = 0; branch < bank->numberBranches; branch++) {
    int berr = Branch_Validate(bank, bank->branches[branch].branchID);
    if (berr < 0) err = berr;
  }
  return err;
}

int
Bank_Compare(Bank *bank1, Bank *bank2)
{
  int err = 0;
  if (bank1->numberBranches != bank2->numberBranches) {
    fprintf(stderr, "Bank num branches mismatch\n");
    return -1;
  }

  for (unsigned int branch = 0; branch < bank1->numberBranches; branch++) {
    int berr = Branch_Compare(&bank1->branches[branch], &bank2->branches[branch]);
    if (berr < 0) err = berr;
  }

  int cerr = Report_Compare(bank1, bank2);
  if (cerr < 0) err = cerr;

  return err;
}
