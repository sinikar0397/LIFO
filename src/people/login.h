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

void login_create_empty_hashtable();
void login_save_hashtable();
void login_update_hashtable(int idx);
int login_load_hashtable();
void login_init();

IdHash login_create_empty_hash();
int login_is_hash_empty(IdHash hash);
int login_is_hash_same(IdHash hash1, IdHash hash2);

ll login_hash_string(char id[], ll base, ll expon);
IdHash login_hash_ID(char id[]);

int login_get_account_offset(char id[]);
int login_does_ID_exist(char id[]);
People* login_get_account(char id[]);

void login_add_hash_to_hashtable(IdHash hash, char id[]);
void login_add_people_tp_hashtable(People* P);
// 백에서 돌릴 함수
// hashTable에 추가해주는 함수

People* login_sign_in_account();
// 얘는 정보 입력받고 계정(Person) 생성하는 함수
// addAcount 호출할거임
// 사실 얘를 직접 쓸 지는 모르겠음
// 일단 테스트 목적으로 추가해둠

People* login_log_in_account();

#endif //LOGIN_H