#include <assert.h>
#include <inttypes.h>
#include <pthread.h>

#include "teller.h"
#include "bank.h"
#include "account.h"
#include "branch.h"
#include "error.h"
#include "debug.h"

static inline void lock_two_accounts(Account *a, Account *b)
{
  if (a == b) { pthread_mutex_lock(&a->lock); return; }
  if (a->accountNumber < b->accountNumber) {
    pthread_mutex_lock(&a->lock);
    pthread_mutex_lock(&b->lock);
  } else {
    pthread_mutex_lock(&b->lock);
    pthread_mutex_lock(&a->lock);
  }
}

static inline void unlock_two_accounts(Account *a, Account *b)
{
  if (a == b) { pthread_mutex_unlock(&a->lock); return; }
  if (a->accountNumber < b->accountNumber) {
    pthread_mutex_unlock(&b->lock);
    pthread_mutex_unlock(&a->lock);
  } else {
    pthread_mutex_unlock(&a->lock);
    pthread_mutex_unlock(&b->lock);
  }
}

int
Teller_DoDeposit(Bank *bank, AccountNumber accountNum, AccountAmount amount)
{
  assert(amount >= 0);

  BranchID bid = AccountNum_GetBranchID(accountNum);
  if (bid >= bank->numberBranches) return ERROR_ACCOUNT_NOT_FOUND;

  Branch *br = &bank->branches[bid];
  pthread_rwlock_wrlock(&br->rwlock);

  Account *acct = Account_LookupByNumber(bank, accountNum);
  if (!acct) { pthread_rwlock_unlock(&br->rwlock); return ERROR_ACCOUNT_NOT_FOUND; }

  pthread_mutex_lock(&acct->lock);
  acct->balance += amount;
  br->balance += amount;
  pthread_mutex_unlock(&acct->lock);

  pthread_rwlock_unlock(&br->rwlock);
  return ERROR_SUCCESS;
}

int
Teller_DoWithdraw(Bank *bank, AccountNumber accountNum, AccountAmount amount)
{
  assert(amount >= 0);

  BranchID bid = AccountNum_GetBranchID(accountNum);
  if (bid >= bank->numberBranches) return ERROR_ACCOUNT_NOT_FOUND;

  Branch *br = &bank->branches[bid];
  pthread_rwlock_wrlock(&br->rwlock);

  Account *acct = Account_LookupByNumber(bank, accountNum);
  if (!acct) { pthread_rwlock_unlock(&br->rwlock); return ERROR_ACCOUNT_NOT_FOUND; }

  pthread_mutex_lock(&acct->lock);

  if (amount > acct->balance) {
    pthread_mutex_unlock(&acct->lock);
    pthread_rwlock_unlock(&br->rwlock);
    return ERROR_INSUFFICIENT_FUNDS;
  }

  acct->balance -= amount;
  br->balance -= amount;

  pthread_mutex_unlock(&acct->lock);
  pthread_rwlock_unlock(&br->rwlock);
  return ERROR_SUCCESS;
}

int
Teller_DoTransfer(Bank *bank, AccountNumber srcNum,
                  AccountNumber dstNum, AccountAmount amount)
{
  assert(amount >= 0);

  Account *src = Account_LookupByNumber(bank, srcNum);
  Account *dst = Account_LookupByNumber(bank, dstNum);
  if (!src || !dst) return ERROR_ACCOUNT_NOT_FOUND;

  BranchID sb = AccountNum_GetBranchID(srcNum);
  BranchID db = AccountNum_GetBranchID(dstNum);

  if (sb == db) {
    lock_two_accounts(src, dst);

    if (amount > src->balance) {
      unlock_two_accounts(src, dst);
      return ERROR_INSUFFICIENT_FUNDS;
    }

    src->balance -= amount;
    dst->balance += amount;

    unlock_two_accounts(src, dst);
    return ERROR_SUCCESS;
  }

  Branch *b1 = &bank->branches[sb];
  Branch *b2 = &bank->branches[db];

  if (sb < db) {
    pthread_rwlock_wrlock(&b1->rwlock);
    pthread_rwlock_wrlock(&b2->rwlock);
  } else {
    pthread_rwlock_wrlock(&b2->rwlock);
    pthread_rwlock_wrlock(&b1->rwlock);
  }

  lock_two_accounts(src, dst);

  if (amount > src->balance) {
    unlock_two_accounts(src, dst);
    if (sb < db) { pthread_rwlock_unlock(&b2->rwlock); pthread_rwlock_unlock(&b1->rwlock); }
    else { pthread_rwlock_unlock(&b1->rwlock); pthread_rwlock_unlock(&b2->rwlock); }
    return ERROR_INSUFFICIENT_FUNDS;
  }

  src->balance -= amount;
  dst->balance += amount;

  bank->branches[sb].balance -= amount;
  bank->branches[db].balance += amount;

  unlock_two_accounts(src, dst);

  if (sb < db) { pthread_rwlock_unlock(&b2->rwlock); pthread_rwlock_unlock(&b1->rwlock); }
  else { pthread_rwlock_unlock(&b1->rwlock); pthread_rwlock_unlock(&b2->rwlock); }

  return ERROR_SUCCESS;
}
