#ifndef LOGIN_H
#define LOGIN_H

#include "./people.h"
#include "../headers.h"

#define PRIME_ENCODING1 1000000007
#define PRIME_ENCODING2 1000000009
#define HASH_SIZE PRIME_HASHING1
#define PRIME_HASHING1  100003
#define PRIME_HASHING2  100019
#define EXPON 128

extern const char* DATA_PATH_PEOPLE;
extern const char* DATA_PATH_HASH;

typedef struct IdHash{
    ll hash1, hash2;
    int offset;
} IdHash;

void saveHashTable();
void updateHashSlot(int idx);
int loadHashTAble();
void initializeLogin();

IdHash emptyHash();
int isEmptyHash(IdHash hash);
int isSameHash(IdHash hash1, IdHash hash2);

ll hashStr(char id[], ll base, ll expon);
IdHash hashID(char id[]);

int getOffset(char id[]);
int existID(char id[]);
People* getAccount(char id[]);

void addHash(IdHash hash, char id[]);
void addAccount(People* P);
// 백에서 돌릴 함수
// hashTable에 추가해주는 함수

People* signInAccount();
// 얘는 정보 입력받고 계정(Person) 생성하는 함수
// addAcount 호출할거임
// 사실 얘를 직접 쓸 지는 모르겠음
// 일단 테스트 목적으로 추가해둠

People* logInAccount();

#endif //LOGIN_H