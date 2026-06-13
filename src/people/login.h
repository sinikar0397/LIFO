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

// @brief ID를 가볍게 들고다니기 위한 구조체
//
// 10^9 정도의 소수로 이차원 해싱을 함, 충돌이 없다고 가정
// 
// 대충 해시테이블 크기가 10^18 정도
//      @param hash1   PRIME_ENCODING1으로 해싱한 값
//      @param hash2   PRIME_ENCODING2으로 해싱한 값
//      @param offset  ID에 대응되는 People 객체가 전체 바이너리 파일에서 저장된 위치
typedef struct IdHash{
    ll hash1, hash2;
    int offset;
} IdHash;


// @brief 프로그램 시작 후 login 내 함수 실행 전에 취해야 하는 함수
//
// Hashtable 불러오기, 없으면 새로 만들어서 저장하기 등등의 역할을 함
void login_init();

// @brief Hashtable의 바이너리 파일을 초기화시킴
void login_create_empty_hashtable();

// @brief 현재 Hasthtable을 모두 바이너리 파일에 저장
void login_save_hashtable();

// @brief Hashtable의 idx번째 데이터를 바이너리 파일에 업데이트
//
// idx번째 데이터는 Hashtable[idx]를 의미
void login_update_hashtable(int idx);

// @brief 전체 Hashtable을 바이너리 파일에서 불러옴
int login_load_hashtable();


// @brief 빈 IdHash 데이터를 생성함
//
// hash1, hash2, offset 모두 -1로 설정됨
//
// IdHash의 NULL data라고 이해해도 좋음
IdHash login_create_empty_hash();

/*
@brief IdHash를 입력받아 empty hash인지 판단
*/
int login_is_hash_empty(IdHash hash);

/*
@brief
두 IdHash를 입력받아 같은 값인지 판단

hash1, hash2만 비교함(offset은 비교 x)
*/
int login_is_hash_same(IdHash hash1, IdHash hash2);


/*
@brief
문자열을 long long으로 해싱해줌

라빈-카프 해싱 이용, 해싱 방법에 큰 의미 없음

@param id 해싱할 데이터
@param base modular 취할 값
@param expon 거듭제곱할 값(EXPON 사용할 것이 권장됨)
*/
ll login_hash_string(char id[], ll base, ll expon);

/*
@brief
문자열(ID라고 가정)을 IdHash로 해싱해줌

라빈-카프 해싱 이용, base는 PRIME_ENCODING1, PRIME_ENCODING2 이용함
*/
IdHash login_hash_ID(char id[]);

/*
@brief
id를 입력받아 HashTable에서 검색, offset을 반환해줌

offset은 전체 사람 바이너리 데이터에서 저장된 위치를 의미

해당 id가 없다면 -1을 반환

login.c 밖에서 사용할 것을 지양할것
*/
int login_get_account_offset(char id[]);

/*
@brief
id를 입력받아 HashTable에 존재하는지 확인해줌

존재하면 1, 없으면 0을 반환
*/
int login_does_ID_exist(char id[]);

/*
@brief
id를 입력받아 전체 사람 바이너리 데이터에서 해당 사람 객체를 읽어줌

Hashtable에서 offset을 찾아온 후, 해당 offset으로 파일을 읽어줌

해당 id가 없으면 warning 띄우고 NULL 반환
*/
People* login_get_account(char id[]);

/*
@brief
IdHash 데이터를 입력받아 해시테이블에 추가해줌

현재 해시테이블에 추가하고 바이너리 파일에 업데이트 해놓음

가능하면 직접 사용하지 말것. login_sign_in_account 사용 권장.
*/
void login_add_hash_to_hashtable(IdHash hash, char id[]);

/*
@brief
People 객체를 입력받아 해시테이블에 Id 정보를 추가해줌

현재 해시테이블에 추가하고 바이너리 파일에도 업데이트 해놓음

가능하면 직접 사용하지 말것. login_sign_in_account 사용 권장.
*/
void login_add_people_to_hashtable(People* P);

/*
@brief
기존 계정 정보를 새 데이터로 저장하고 Hashtable offset을 최신 위치로 갱신함
*/
int login_update_account(People* P);

/*
@brief
자체 I/O를 기반으로 객체 정보를 만들고 People 객체를 생성함

해시테이블에 관련 정보를 추가하고, 해시테이블 바이너리 파일과 사람 객체 바이너리 파일에도 업데이트함

향후 standard I/O가 아니라 dfs(Dating Fit survey)를 이용해 성격 유형 입력받을 에정
*/
People* login_sign_in_account();

/*
@brief
자체 I/O를 기반으로 id를 입력받은 후, 바이너리 파일에서 해당 사람 객체를 찾아서 반환해줌
*/
People* login_log_in_account();

#endif //LOGIN_H
