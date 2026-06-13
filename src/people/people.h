#ifndef PEOPLE_H
#define PEOPLE_H

#include "../headers.h"
#include <stdint.h>

#define MAX_NAME_LEN 20
#define MAX_ID_LEN 40
#define MAX_PW_LEN 40
#define MAX_TYPE_LEN 20
#define MAX_PATH_LEN 50
#define MAX_BLOCKED 50

#define LEFTROTATE(x, c) (((x) << (c)) | ((x) >> (32 - (c))))

extern uint32_t k[64];
extern uint32_t s[64];

/*
@breif
성별 저장하는 객체

GENDER_MALE, GENDER_FEMALE밖에 없음
*/
enum Gender { GENDER_MALE, GENDER_FEMALE };

/*
@brief
비밀번호 해싱한 값 저장하는 객체

MD5를 이용해 16바이트으로 저장함
*/
typedef struct Password {
	uint8_t digest[16];
} Password;

/*
 * 매칭 상태를 나타내는 enum
 * AVAILABLE : 매칭 알고리즘에 참여 가능한 상태
 * PROPOSED  : 추천 쌍으로 선정되었지만 아직 양쪽 수락 전
 * MATCHED   : 양쪽이 수락하여 최종 매칭된 상태
 * PAUSED    : 매칭 추천을 일시 중단한 상태
 * DELETED   : 탈퇴 또는 삭제된 사용자
 */
typedef enum MatchStatus {
	AVAILABLE,
	PROPOSED,
	MATCHED,
	PAUSED,
	DELETED
} MatchStatus;

/*
@brief
사람(계정)을 저장하는 객체

이름, id, 비번, 성격 유형, 이상형 성격 유형, 성별, 나이 저장함

비밀번호는 암호화하여 저장됨
*/
typedef struct People {
	char name[MAX_NAME_LEN];
	char id[MAX_ID_LEN];
	Password pw;
	char type[MAX_TYPE_LEN];
	char love_type[MAX_TYPE_LEN];
	char attach[MAX_TYPE_LEN];	   // 내 애착 유형 (AS/AV/AX/FA), 미설정 시 ""
	char love_attach[MAX_TYPE_LEN]; // 이상형의 애착 유형, 미설정 시 ""
	char lang[MAX_TYPE_LEN];	   // 내 사랑의 언어 (LW/LT/LQ/LA), 미설정 시 ""
	char love_lang[MAX_TYPE_LEN];  // 이상형의 사랑의 언어, 미설정 시 ""
	enum Gender gen;
	int age;
	MatchStatus status;
	char blocked_ids[MAX_BLOCKED][MAX_ID_LEN];
    int blocked_cnt;
	// 세분화 1회 제한용 세션 플래그. 직렬화하지 않으며, 로그인 시 0으로
	// 초기화됨(people_create_people). 재설문하면 다시 0으로 풀린다.
	int dfs_extended;
} People;

/*
@brief
Password 객체의 암호화문을 출력해줌

사실상 디버깅용 함수
*/
void people_print_password(Password pw);

/*
@brief
문자열을 입력받아서 md5 해싱을 이용해 Password 객체로 암호화해줌
*/
Password people_hash_password(char pw[]);

/*
@brief
암호화된 문자열을 받아서 Password 객체로 변환해줌

그냥 typecasting으로 이해해도 무방
*/
Password people_read_password_from_hex(char pw_hex[]);
;

/*
@brief
두 비밀번호를 입력받아서 같은지(해싱 결과가 같은지) 확인해줌
*/
int people_is_same_password(Password p1, Password p2);

/*
@brief
People에 대한 생성자

각각의 항목을 입력받아 People 객체를 생성해줌

비밀번호의 경우, 암호화하지 않은 단순 문자열을 입력하면 됨
*/
People *people_create_people(char name[], char id[], char pw[], char type[],
							 char love_type[], enum Gender gen, int age);

/*
@brief
People의 이름에 대한 setter
*/
void people_set_people_name(People *P, char name[]);

/*
@brief
People의 id에 대한 setter
*/
void people_set_people_id(People *P, char id[]);

/*
@brief
People의 비밀번호에 대한 setter

암호화하지 않은 평문(단순 문자열)을 입력하면 됨
*/
void people_set_people_pw(People *P, char pw[]);

/*
@brief
People의 status에 대한 setter
*/
void people_set_people_status(People* P, MatchStatus status);

/*
@brief
People의 비밀번호에 대한 setter

암호화된 Password 객체를 입력하면 됨
*/
void people_set_people_pw_hashed(People *P, Password pw);

/*
@brief
People의 성격유형에 대한 setter
*/
void people_set_people_type(People *P, char type[]);

/*
@brief
People의 이상형의 성격유형에 대한 setter
*/
void people_set_people_love_type(People *P, char love_type[]);

/*
@brief
People의 애착 유형에 대한 setter
*/
void people_set_people_attach(People *P, char attach[]);

/*
@brief
People의 이상형의 애착 유형에 대한 setter
*/
void people_set_people_love_attach(People *P, char love_attach[]);

/*
@brief
People의 사랑의 언어에 대한 setter
*/
void people_set_people_lang(People *P, char lang[]);

/*
@brief
People의 이상형의 사랑의 언어에 대한 setter
*/
void people_set_people_love_lang(People *P, char love_lang[]);

/*
@brief
People의 성별에 대한 setter
*/
void people_set_people_gen(People *P, enum Gender gen);

/*
@brief
People의 나이에 대한 setter
*/
void people_set_people_age(People *P, int age);

/*
@brief
사람 객체들이 적힌 jsonl파일에서 한 줄(한 객체)을 읽어와주는 함수

offset비트의 데이터부터 \n까지의 데이터를 읽어옴

ex. offset이 13인 경우 13번째 글자부터 해당 줄을 읽어옴

people_read_people등을 위한 함수임, 그 외의 분야에서 쓰는 것을 지양할 것

@param path jsonl 파일의 경로
@param offset 파일 읽기 시작할 위치
*/
char *people_read_text_from_file(const char path[], int offset);

/*
@brief
사람 객체들이 적힌 jsonl파일에서 한 줄(한 객체)을 읽어 People 객체를 생성해주는
함수

offset비트의 데이터부터 \n까지의 데이터를 읽어옴

ex. offset이 13인 경우 13번째 글자부터 해당 줄을 읽어옴

이 함수를 login.c오의 장소에서 쓰는 것을 지양할 것.

login.c의 login_log_in_account 함수 이용을 권장함.

@param path jsonl 파일의 경로
@param offset 파일 읽기 시작할 위치
*/
People *people_read_people(const char path[], int offset);

/*
@brief
People 객체를 받아 jsonl 파일에 저장해주는 함수

저장된 위치의 offset을 반환해줌

단, 해당 함수를 이용할 경우 해시테이블에 추가되지 않음.

login.c의 login_add_people_to_hashtable 함수 이용을 권장함.

@param path jsonl 파일의 경로
@param offset 파일 읽기 시작할 위치
*/
int people_save_people(People *P, const char path[]);

void people_print_people(People *P);
void people_delete_people(People *P);

#endif // PEOPLE_H
