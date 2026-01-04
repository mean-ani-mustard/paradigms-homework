#ifndef _BRANCH_H
#define _BRANCH_H

#include <stdint.h>
#include <pthread.h>

#include "account.h"   

typedef uint64_t BranchID;

struct Bank;

typedef struct Branch {
  BranchID branchID;
  AccountAmount balance;
  int numberAccounts;
  Account *accounts;
  pthread_rwlock_t rwlock;   
} Branch;

static inline void Branch_RdLock(Branch *b) { pthread_rwlock_rdlock(&b->rwlock); }
static inline void Branch_WrLock(Branch *b) { pthread_rwlock_wrlock(&b->rwlock); }
static inline void Branch_Unlock(Branch *b) { pthread_rwlock_unlock(&b->rwlock); }

int Branch_Balance(struct Bank *bank, BranchID branchID, AccountAmount *balance);
int Branch_UpdateBalance(struct Bank *bank, BranchID branchID, AccountAmount change);

int Branch_Init(struct Bank *bank, int numBranches, int numAccounts,
                AccountAmount initialAmount);

int Branch_Validate(struct Bank *bank, BranchID branchID);
int Branch_Compare(Branch *branch1, Branch *branch2);

#endif /* _BRANCH_H */
