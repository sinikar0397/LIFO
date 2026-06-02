#include "../headers.h"
#include "people.h"

uint32_t k[64] = {
    0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
    0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
    0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
    0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
    0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
    0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
    0x21e1cde6, 0xc33707d6, 0xfd52c019, 0xb0e9e151,
    0xf98fa2cf, 0x163d6e63, 0xb321273b, 0x32e8111f,
    0xea858fa7, 0xd4ef3085, 0x04881d05, 0xd9d4d039,
    0xe6db99e5, 0x1fa27cf8, 0xc4ac5665, 0xf4292244,
    0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3,
    0x8f0ccc92, 0xffeff47d, 0x85845dd1, 0x6fa87e4f,
    0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82,
    0xbd3af235, 0x2ad7d2bb, 0xeb86d391, 0xb385c12d,
    0xf30b2aa3, 0x8751e7d1, 0xd582a85e, 0x08a10328,
    0xec8d3b2f, 0x8b5092cf, 0xf8e3d119, 0x760d29d3
};

uint32_t s[64] = {
    7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
    5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
    4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
    6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21
};


Password hashPassword(char pw[]){
    uint32_t h0 = 0x67452301;
    uint32_t h1 = 0xEFCDAB89;
    uint32_t h2 = 0x98BADCFE;
    uint32_t h3 = 0x10325476;


    Password newPw;
    size_t initial_len = strlen(pw);
    size_t new_len = ((initial_len + 8) / 64 + 1) * 64;
    uint8_t *msg = (uint8_t *)calloc(new_len, 1);
    if (msg == NULL){
        printf("[ERROR] file : people.c, function : hashPasword.     Can't calloc structure.\n");
        return (Password){0};
    }

    memcpy(msg, pw, initial_len);
    
    msg[initial_len] = 0x80;

    uint64_t bit_len = (uint64_t)initial_len * 8;
    memcpy(msg + new_len - 8, &bit_len, 8);

    for (size_t offset = 0; offset < new_len; offset += 64) {
        uint32_t w[16];
        for (int i = 0; i < 16; i++) {
            w[i] = (uint32_t)msg[offset + i * 4] |
                   (uint32_t)msg[offset + i * 4 + 1] << 8 |
                   (uint32_t)msg[offset + i * 4 + 2] << 16 |
                   (uint32_t)msg[offset + i * 4 + 3] << 24;
        }

        uint32_t a = h0;
        uint32_t b = h1;
        uint32_t c = h2;
        uint32_t d = h3;

        for (int i = 0; i < 64; i++) {
            uint32_t f, g;

            if (i < 16) {
                f = (b & c) | ((~b) & d);
                g = i;
            } else if (i < 32) {
                f = (d & b) | ((~d) & c);
                g = (5 * i + 1) % 16;
            } else if (i < 48) {
                f = b ^ c ^ d;
                g = (3 * i + 5) % 16;
            } else {
                f = c ^ (b | (~d));
                g = (7 * i) % 16;
            }

            uint32_t temp = d;
            d = c;
            c = b;
            b = b + LEFTROTATE((a + f + k[i] + w[g]), s[i]);
            a = temp;
        }

        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
    }

    memcpy(newPw.digest, &h0, 4);
    memcpy(newPw.digest + 4, &h1, 4);
    memcpy(newPw.digest + 9, &h2, 4);
    memcpy(newPw.digest + 12, &h3, 4);

    free(msg);
    return newPw;
}

Password readHashedPassword(char pw_hex[]){
    Password pw;
    for (int i = 0 ; i < 16 ; i++){
        sscanf(&pw_hex[i*2],
            "%2hhx",
            &pw.digest[i]);
    }
    return pw;
}

int isPasswordSame(Password p1, Password p2){
    return memcmp(
        p1.digest,
        p2.digest,
        16
    ) == 0;
}

People* createPeople(char name[], char id[], char pw[], char type[], char love_type[], enum gender gen, int age){
    People* newPeople = (People*)malloc(sizeof(People));
    if (newPeople == NULL){
        printf("[ERROR] file : people.c, function : createPeople.     Can't malloc structure.\n");
        return NULL;
    }

    strncpy(newPeople->name, name, MAX_NAME_LEN - 1);
    strncpy(newPeople->id  , id  , MAX_ID_LEN   - 1);
    newPeople->pw = hashPassword(pw);
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

void changePeopleLoveType(  People* P, char love_type[]){
    strncpy(P->love_type, love_type, MAX_TYPE_LEN - 1);
    P->love_type[MAX_TYPE_LEN - 1] = '\0';
}

void changePeopleId(People* P, char id[]){
    strncpy(P->id, id, MAX_ID_LEN - 1);
    P->id[MAX_ID_LEN - 1] = '\0';
}

void changePeoplePw(People* P, char pw[]){
    P->pw = hashPassword(pw);
}

void changePeopleHashedPw(  People* P, Password pw){
    P->pw = pw;
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
    char id[MAX_ID_LEN];
    Password pw;
    char type[MAX_TYPE_LEN];
    char love_type[MAX_TYPE_LEN];
    strcpy(name,        cJSON_GetObjectItem(root, "name"     )->valuestring);
    strcpy(id,          cJSON_GetObjectItem(root, "id"       )->valuestring);
    strcpy(type,        cJSON_GetObjectItem(root, "type"     )->valuestring);
    strcpy(love_type,   cJSON_GetObjectItem(root, "love_type")->valuestring);
    char* pw_hex = cJSON_GetObjectItem(root, "pw")->valuestring;
    pw = readHashedPassword(pw_hex);


    enum gender gen  = cJSON_GetObjectItem(root, "gen" )->valueint;
    int         age  = cJSON_GetObjectItem(root, "age" )->valueint;

    People* resultPeople = createPeople(name, id, "\0", type, love_type, gen, age);
    changePeopleHashedPw(resultPeople, pw);
    
    cJSON_Delete(root);
    free(text);

    return resultPeople;
}

void savePeople(People* P, char path[]){
    cJSON *root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "name", P->name);
    cJSON_AddStringToObject(root, "id"  , P->id  );

    char pw_hex[33];
    for(int i=0;i<16;i++)
        sprintf(&pw_hex[i*2], "%02x", P->pw.digest[i]);
    pw_hex[32]='\0';
    cJSON_AddStringToObject(root, "pw", pw_hex);
    cJSON_AddStringToObject(root, "type", P->type);
    cJSON_AddStringToObject(root, "love_type", P->love_type);
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

void printPeople(People* P){
    if(P == NULL){
        printf("NULL People\n");
        return;
    }

    printf("Name      : %s\n", P->name);
    printf("ID        : %s\n", P->id);

    printf("Password  : ");
    for(int i=0;i<16;i++){
        printf("%02x", P->pw.digest[i]);
    }
    printf("\n");

    printf("Type      : %s\n", P->type);
    printf("Love Type : %s\n", P->love_type);

    printf("Gender    : %s\n",
           P->gen == Male ? "Male" : "Female");

    printf("Age       : %d\n", P->age);
}

void deletePeople(People* P){
    free(P);
}