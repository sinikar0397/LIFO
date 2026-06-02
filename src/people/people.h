#ifndef PEOPLE_H
#define PEOPLE_H

#define MAX_NAME_LEN 20
#define MAX_ID_LEN 20
#define MAX_PW_LEN 20
#define MAX_TYPE_LEN 20
#define MAX_PATH_LEN 50

#define LEFTROTATE(x, c) (((x) << (c)) | ((x) >> (32 - (c))))

extern uint32_t k[64];
extern uint32_t s[64];

enum gender {
    MALE,
    FEMALE
};

typedef struct Password{
    uint8_t digest[16];
} Password;

Password hashPassword(char pw[]);
Password readHashedPassword(char pw_hex[]);;
int isPasswordSame(Password p1, Password p2);

typedef struct People {
    char name[MAX_NAME_LEN];
    char id[MAX_ID_LEN];
    Password pw;
    char type[MAX_TYPE_LEN];
    char love_type[MAX_TYPE_LEN];
    enum gender gen;
    int age;
} People;

People* createPeople(char name[], char id[], char pw[], char type[], char love_type[], enum gender gen, int age);
void changePeopleName(      People* P, char name[]);
void changePeopleId(        People* P, char id[]);
void changePeoplePw(        People* P, char pw[]);
void changePeopleHashedPw(  People* P, Password pw);
void changePeopleType(      People* P, char type[]);
void changePeopleLoveType(  People* P, char love_type[]);
void changePeopleGen(       People* P, enum gender gen);
void changePeopleAge(       People* P, int age);

char*   readFile(  char path[]);
People* readPeople(char path[]);
void    savePeople(People* P, char path[]);

void printPeople(People* P);
void deletePeople(People* P);

#endif //PEOPLE_H
