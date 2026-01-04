#ifndef _ACCOUNT_H
#define _ACCOUNT_H

#include <stdint.h>
#include <pthread.h>



typedef uint64_t AccountNumber;
typedef int64_t AccountAmount;

struct Bank;


typedef struct Account {
  AccountNumber accountNumber;
  AccountAmount balance;
  pthread_mutex_t lock;
} Account;

static inline void Account_Lock(Account *a)   { pthread_mutex_lock(&a->lock); }
static inline void Account_Unlock(Account *a) { pthread_mutex_unlock(&a->lock); }



Account *Account_LookupByNumber(struct Bank *bank, AccountNumber accountNum);

void Account_Adjust(struct Bank *bank, Account *account,
                    AccountAmount amount,
                    int updateBranch);

AccountAmount Account_Balance(Account *account);

AccountNumber Account_MakeAccountNum(int branch, int subaccount);

int Account_IsSameBranch(AccountNumber accountNum1, AccountNumber accountNum2);

void Account_Init(struct Bank *bank, Account *account, int id, int branch,
                  AccountAmount initialAmount);
uint64_t AccountNum_GetBranchID(AccountNumber accountNum);


#endif /* _ACCOUNT_H */
