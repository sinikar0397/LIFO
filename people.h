#ifndef PEOPLE_H
#define PEOPLE_H

#define MAX_NAME_LEN 20
#define MAX_TYPE_LEN 20
#define MAX_PATH_LEN 50

enum gender {
    Male,
    Female
};

typedef struct People {
    char name[MAX_NAME_LEN];
    char type[MAX_TYPE_LEN];
    enum gender gen;
    int age;
} People;

People* createPeople(char name[], char type[], enum gender gen, int age);
void changePeopleName(People* P, char name[]);
void changePeopleType(People* P, char type[]);
void changePeopleGen( People* P, enum gender gen);
void changePeopleAge( People* P, int age);

char*   readFile(  char path[]);
People* readPeople(char path[]);
void    savePeople(People* P, char path[]);

void printPeople(People* P);

#endif //PEOPLE_H
