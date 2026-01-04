#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>
#include <pthread.h>

#include "bank.h"
#include "branch.h"
#include "account.h"
#include "debug.h"

int
Branch_Init(struct Bank *bank, int numBranches, int numAccounts,
            AccountAmount initialAmount)
{
  bank->numberBranches = numBranches;
  bank->branches = malloc(numBranches * sizeof(Branch));
  if (bank->branches == NULL) return -1;

  int accountsPerBranch = numAccounts / numBranches;

  for (int i = 0; i < numBranches; i++) {
    Branch *branch = &bank->branches[i];

    branch->branchID = i;
    branch->balance = 0;
    branch->numberAccounts = accountsPerBranch;
    branch->accounts = (Account *)malloc(accountsPerBranch * sizeof(Account));
    if (branch->accounts == NULL) return -1;

    pthread_rwlock_init(&branch->rwlock, NULL);

    for (int a = 0; a < accountsPerBranch; a++) {
      Account_Init(bank, &branch->accounts[a], a, i, initialAmount);
      branch->balance += branch->accounts[a].balance;
    }
  }

  return 0;
}

int
Branch_UpdateBalance(struct Bank *bank, BranchID branchID, AccountAmount change)
{
  assert(bank->branches);
  Y;
  if (branchID >= bank->numberBranches) return -1;

  Branch *b = &bank->branches[branchID];
  pthread_rwlock_wrlock(&b->rwlock);
  b->balance += change;
  pthread_rwlock_unlock(&b->rwlock);
  return 0;
}

int
Branch_Balance(struct Bank *bank, BranchID branchID, AccountAmount *balance)
{
  assert(bank->branches);
  if (branchID >= bank->numberBranches) return -1;

  Branch *b = &bank->branches[branchID];
  pthread_rwlock_rdlock(&b->rwlock);
  *balance = b->balance;
  pthread_rwlock_unlock(&b->rwlock);
  Y;
  return 0;
}

int
Branch_Validate(struct Bank *bank, BranchID branchID)
{
  assert(bank->branches);
  if (branchID >= bank->numberBranches) return -1;

  Branch *branch = &bank->branches[branchID];
  AccountAmount total = 0;

  for (int a = 0; a < branch->numberAccounts; a++) total += branch->accounts[a].balance;

  if (total != branch->balance) {
    fprintf(stderr, "Branch balance mismatch. Computer value is %"PRId64", but stored value is %"PRId64"\n",
            total, branch->balance);
    return -1;
  }
  return 0;
}

int
Branch_Compare(Branch *branch1, Branch *branch2)
{
  int err = 0;

  BranchID branch1ID = branch1->branchID;
  BranchID branch2ID = branch2->branchID;

  if (branch1->numberAccounts != branch2->numberAccounts) {
    fprintf(stderr, "Branches %"PRIu64" and %"PRIu64" mismatch in numberAccounts (%d and %d, respectively).\n",
            branch1ID, branch2ID, branch1->numberAccounts, branch2->numberAccounts);
    err = -1;
  }

  if (branch1->balance != branch2->balance) {
    fprintf(stderr, "Branches %"PRIu64" and %"PRIu64" mismatch in balance (%"PRId64" and %"PRId64", respectively).\n",
            branch1ID, branch2ID, branch1->balance, branch2->balance);
    err = -1;
  }

  for (int i = 0; i < branch1->numberAccounts; i++) {
    assert(branch1->accounts[i].accountNumber == branch2->accounts[i].accountNumber);
    if (branch1->accounts[i].balance != branch2->accounts[i].balance) {
      fprintf(stderr,
              "Branch %"PRIu64" and %"PRIu64" mismatch in account 0x%"PRIx64" balance (%"PRId64" and %"PRId64", respectively).\n",
              branch1ID, branch2ID, branch1->accounts[i].accountNumber,
              branch1->accounts[i].balance, branch2->accounts[i].balance);
      err = -1;
    }
  }
  return err;
}
