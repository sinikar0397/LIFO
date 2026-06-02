#include "headers.h"
#include "people.h"


Password hashPassword(char pw[]){
    //todo 
}

int isPasswordSame(Password p1, Password p2){
    //todo
}



People* createPeople(char name[], char id[], char pw[], char type[], char love_type[], enum gender gen, int age){
    People* newPeople = (People*)malloc(sizeof(People));
    if (newPeople == NULL){
        printf("[ERROR] file : people.c, function : createPeople.     Can't malloc structure.\n");
        return NULL;
    }

    strncpy(newPeople->name, name, MAX_NAME_LEN - 1);
    strncpy(newPeople->id  , id  , MAX_ID_LEN   - 1);
    pw = hashPassword(char pw[]);
    strncpy(newPeople->type, type, MAX_TYPE_LEN - 1);
    strncpy(newPeople->love_type, love_type, MAX_TYPE_LEN - 1);
    newPeople->name[MAX_NAME_LEN - 1] = '\0';
    newPeople->id[  MAX_ID_LEN   - 1] = '\0';
    newPeople->type[MAX_TYPE_LEN - 1] = '\0';
    newPeople->love_type[MAX_TYPE_LEN - 1] = '\0';
    newPeople->gen  = gen;
    newPeople->age  = age;
    return newPeople;
}

void changePeopleName(People* P, char name[]){
    strncpy(P->name, name, MAX_NAME_LEN - 1);
    P->name[MAX_NAME_LEN - 1] = '\0';
}

void changePeopleType(People* P, char type[]){
    strncpy(P->type, type, MAX_TYPE_LEN - 1);
    P->type[MAX_TYPE_LEN - 1] = '\0';
}

void changePeopleId(People* P, char id[]){
    stncpy(P->id, id, MAX_ID_LEN - 1);
    P->id[MAX_ID_LEN - 1] = '\0';
}

void changePeoplePw(People* P, char pw[]){
    P->pw = hashPassword(pw);
}

void changePeopleGen( People* P, enum gender gen){
    P->gen = gen;
}
void changePeopleAge( People* P, int age){
    P->age = age;
}

char* readFile(char path[]){
    FILE *fp = fopen(path, "r");
    if (fp == NULL){
        printf("[ERROR] file : people.c, function : readFile.     Can't read file. Probably Wrong Path\n");
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);

    char* buffer = (char*)malloc(size + 1);
    fread(buffer, 1, size, fp);
    buffer[size] = '\0';

    fclose(fp);
    return buffer;
}

People* readPeople(char path[]){
    char* text = readFile(path);
    if (text == NULL){
        printf("[ERROR] file : people.c, function : readPeople.     Can't read text from readFile. Probably Error hold on readFile.\n");
        return NULL;
    }

    cJSON* root = cJSON_Parse(text);
    if (root == NULL){
        printf("[ERROR] file : people.c, function : readPeople.     Can't parse text from text.\n");
        free(text);
        return NULL;
    }

    char name[MAX_NAME_LEN];
    char type[MAX_TYPE_LEN];
    strcpy(name, cJSON_GetObjectItem(root, "name")->valuestring);
    strcpy(type, cJSON_GetObjectItem(root, "type")->valuestring);

    enum gender gen  = cJSON_GetObjectItem(root, "gen" )->valueint;
    int         age  = cJSON_GetObjectItem(root, "age" )->valueint;

    People* resultPeople = createPeople(name, type, gen, age);
    
    cJSON_Delete(root);
    free(text);

    return resultPeople;
}

void savePeople(People* P, char path[]){
    cJSON *root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "name", P->name);
    cJSON_AddStringToObject(root, "type", P->type);
    cJSON_AddNumberToObject(root, "gen" , P->gen );
    cJSON_AddNumberToObject(root, "age" , P->age );

    char* json_string = cJSON_Print(root);
    
    FILE *fp = fopen(path, "w");
    if (fp == NULL){
        printf("[ERROR] file : people.c, function : savePeople.    Can't access file to path.\n");
        free(json_string);
        cJSON_Delete(root);
        return;
    }

    fprintf(fp, "%s", json_string);
    fclose(fp);

    free(json_string);
    cJSON_Delete(root);
}
