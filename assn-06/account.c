#include <assert.h>
#include <stdint.h>
#include <pthread.h>

#include "bank.h"
#include "branch.h"
#include "account.h"
#include "debug.h"

extern int testfailurecode;

BranchID
AccountNum_GetBranchID(AccountNumber accountNum)
{
  Y;
  return (BranchID)(accountNum >> 32);
}

static int
AcountNum_Subaccount(AccountNumber accountNum)
{
  Y;
  return (int)(accountNum & 0x7ffffff);
}

AccountNumber
Account_MakeAccountNum(int branch, int subaccount)
{
  AccountNumber num = (AccountNumber)subaccount;
  num |= ((uint64_t)branch) << 32;
  Y;
  return num;
}

int
Account_IsSameBranch(AccountNumber accountNum1, AccountNumber accountNum2)
{
  return (AccountNum_GetBranchID(accountNum1) ==
          AccountNum_GetBranchID(accountNum2));
}

Account *
Account_LookupByNumber(struct Bank *bank, AccountNumber accountNum)
{
  BranchID branchID = AccountNum_GetBranchID(accountNum);
  int branchIndex = AcountNum_Subaccount(accountNum);
  return &(bank->branches[branchID].accounts[branchIndex]);
}

void
Account_Init(struct Bank *bank, Account *account, int id, int branch,
             AccountAmount initialAmount)
{
  account->accountNumber = Account_MakeAccountNum(branch, id);
  account->balance = initialAmount;

  if (testfailurecode) {
    if ((id & 0x3) == 0) account->balance = -1;
  }

  pthread_mutex_init(&account->lock, NULL);
}

AccountAmount
Account_Balance(Account *account)
{
  AccountAmount b;
  pthread_mutex_lock(&account->lock);
  b = account->balance;
  pthread_mutex_unlock(&account->lock);
  Y;
  return b;
}

void
Account_Adjust(struct Bank *bank, Account *account, AccountAmount amount, int updateBranch)
{
  BranchID bid = AccountNum_GetBranchID(account->accountNumber);

  if (updateBranch) pthread_rwlock_wrlock(&bank->branches[bid].rwlock);

  pthread_mutex_lock(&account->lock);
  account->balance = account->balance + amount;
  pthread_mutex_unlock(&account->lock);

  if (updateBranch) {
    bank->branches[bid].balance += amount;
    pthread_rwlock_unlock(&bank->branches[bid].rwlock);
  }

  Y;
}
