#ifndef PEOPLE_H
#define PEOPLE_H

#include "../headers.h"

#define MAX_NAME_LEN 20
#define MAX_ID_LEN 20
#define MAX_PW_LEN 20
#define MAX_TYPE_LEN 20
#define MAX_PATH_LEN 50

#define LEFTROTATE(x, c) (((x) << (c)) | ((x) >> (32 - (c))))

extern uint32_t k[64];
extern uint32_t s[64];

enum gender {
    GENDER_MALE,
    GENDER_FEMALE
};

typedef struct Password{
    uint8_t digest[16];
} Password;

typedef struct People {
    char name[MAX_NAME_LEN];
    char id[MAX_ID_LEN];
    Password pw;
    char type[MAX_TYPE_LEN];
    char love_type[MAX_TYPE_LEN];
    enum gender gen;
    int age;
} People;

void people_print_password(Password pw);
Password people_hash_password(char pw[]);
Password people_read_password_from_hex(char pw_hex[]);;
int people_is_same_password(Password p1, Password p2);

People* people_create_people(char name[], char id[], char pw[], char type[], char love_type[], enum gender gen, int age);
void people_set_people_name(People* P, char name[]);
void people_set_people_id(People* P, char id[]);
void people_set_people_pw(People* P, char pw[]);
void people_set_people_pw_hashed(People* P, Password pw);
void people_set_people_type(People* P, char type[]);
void people_set_people_love_type(People* P, char love_type[]);
void people_set_people_gen(People* P, enum gender gen);
void people_set_people_age(People* P, int age);

char* people_read_text_from_file(const char path[], int offset);

// offset : 전체 jsonl 파일에서 이게 몇번째 데이터인지.
// 이 함수 직접 쓰지 마세요 login.h에 id 넣으면 People 반환하는 함수 만들어둘 예정임
People* people_read_people(const char path[], int offset);

// 반환 : 마지막에 저장한 offset 위치
// 웬만하면 이 코드도 직접 실행하지 말것. login.h에서 addAcount 쓰세요
int people_save_people(People* P, const char path[]);

void people_print_people(People* P);
void people_delete_people(People* P);

#endif //PEOPLE_H
