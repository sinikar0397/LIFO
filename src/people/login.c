#include "login.h"
#include "people.h"

const char* DATA_PATH_PEOPLE = "./database/peoples/data.jsonl";
const char* DATA_PATH_HASH = "./database/peoples/hashTable.dat";

IdHash HashTable[HASH_SIZE];

void createEmptyHashFile(){
    FILE* fp = fopen(DATA_PATH_HASH, "wb");

    if(fp == NULL)
        return;

    IdHash empty = emptyHash();

    for(int i=0;i<HASH_SIZE;i++)
        fwrite(&empty, sizeof(IdHash), 1, fp);

    fclose(fp);
}

void saveHashTable(){
    FILE* fp = fopen(DATA_PATH_HASH, "wb");
    if(fp == NULL){
        printf("[ERROR] file : login.c, function : saveHashTable.       Can't open hash file.\n");
        return;
    }
    fwrite(
        HashTable,
        sizeof(IdHash),
        HASH_SIZE,
        fp
    );
    fclose(fp);
}

void updateHashSlot(int idx){
    FILE* fp = fopen(DATA_PATH_HASH, "rb+");
    if(fp == NULL)
        return;
    
    fseek(
        fp,
        idx * sizeof(IdHash),
        SEEK_SET
    );
    fwrite(
        &HashTable[idx],
        sizeof(IdHash),
        1,
        fp
    );

    fclose(fp);
}

int loadHashTable(){
    FILE* fp = fopen(DATA_PATH_HASH, "rb");

    if(fp == NULL)
        return 0;

    size_t count = fread(
        HashTable,
        sizeof(IdHash),
        HASH_SIZE,
        fp
    );

    fclose(fp);

    return count == HASH_SIZE;
}

void initializeLogin(){
    if(loadHashTable())
        return;

    for(int i=0;i<HASH_SIZE;i++)
        HashTable[i] = emptyHash();

    createEmptyHashFile();
}

IdHash emptyHash(){
    IdHash result;
    result.hash1 = -1;
    result.hash2 = -1;
    result.offset = -1;
    return result;
}

int isEmptyHash(IdHash hash){
    return (hash.hash1 == -1 && hash.hash2 ==-1);
}

int isSameHash(IdHash hash1, IdHash hash2){
    return (hash1.hash1 == hash2.hash1 && hash1.hash2 == hash2.hash2);
}

ll hashStr(char id[], ll base, ll expon){
    ll res = 0;
    for (char* ptr = id; *ptr != '\0'; ptr++){
        res = (res * expon + *ptr) % base;
    }
    return res;
}

IdHash hashID(char id[]){
    IdHash result_hash;
    result_hash.hash1 = hashStr(id, PRIME_ENCODING1, EXPON);
    result_hash.hash2 = hashStr(id, PRIME_ENCODING2, EXPON);
    return result_hash;
}

int getOffset(char id[]){
    IdHash hashed_id = hashID(id);
    ll h1 = hashStr(id, PRIME_HASHING1, EXPON);
    ll h2 = hashStr(id, PRIME_HASHING2, EXPON);

    int idx = h1;
    int cnt = 0;
    while (cnt++ <= HASH_SIZE){
        if (isEmptyHash(HashTable[idx]))
            return -1;
        if (isSameHash(HashTable[idx], hashed_id))
            return HashTable[idx].offset;
        idx = (idx + h2) % HASH_SIZE;
    }
    printf("[ERROR] file : login.c, function : getOffset.     HashTable is full(almost, probably.). Check The Hash Table.\n");
    return -1;
}

int existID(char id[]){
    return getOffset(id) != -1;
}

People* getAccount(char id[]){
    int offset = getOffset(id);
    return readPeople(DATA_PATH_PEOPLE, offset);
}

void addHash(IdHash hash, char id[]){
    ll h1 = hashStr(id, PRIME_HASHING1, EXPON);
    ll h2 = hashStr(id, PRIME_HASHING2, EXPON);

    int idx = h1;
    int cnt = 0;
    while (cnt++ <= HASH_SIZE){
        if (isEmptyHash(HashTable[idx])){
            HashTable[idx] = hash;
            updateHashSlot(idx);
            return;
        }
        idx = (idx + h2) % HASH_SIZE;
    }
    printf("[ERROR] file : login.c, function : addHash.     Saving failed. HashTable is full(almost, probably.). Check The Hash Table.\n");
}

void addAccount(People* P){
    int offset = savePeople(P, DATA_PATH_PEOPLE);
    IdHash hash = hashID(P->id);
    hash.offset = offset;
    addHash(hash, P->id);
}

People* signInAccount(){
    char name[MAX_NAME_LEN];
    char id[MAX_ID_LEN];
    char pw[MAX_PW_LEN];
    
    char type[MAX_TYPE_LEN];
    char love_type[MAX_TYPE_LEN];
    enum gender gen;
    int age;

    printf("ID를 입력해주십시오(띄어쓰기 없이 입력하십시오.) : ");
    scanf("%s", id);
    while (existID(id)){
        printf("이미 존재하는 ID입니다. 다른 ID를 입력해주십시오 : ");
        scanf("%s", id);
    }
    printf("비밀번호를 입력해주십시오(띄어쓰기 없이 입력하십시오.) : ");
    scanf("%s", pw);
    
    printf("이름을 입력해주십시오 : ");
    scanf("%s", name);

    printf("성격 유형을 입력해주십시오 : ");
    scanf("%s", type);

    printf("좋아하는 성격 유형을 입력해주십시오 : ");
    scanf("%s", love_type);

    printf("성별을 입력해주십시오(남자면 0, 여자면 1로 입력하십시오.) : ");
    scanf("%d", &gen);

    printf("나이를 입력해주십시오 : ");
    scanf("%d", &age);

    People* resultPeople = createPeople(
        name, id, pw, type, love_type, gen, age
    );
    addAccount(resultPeople);
    return resultPeople;
}

People* logInAccount(){
    char id[MAX_ID_LEN];
    char pw[MAX_PW_LEN];
    Password HashedPassword;
    People* answerAccount;

    while (1){
        printf("ID를 입력해주십시오(띄어쓰기 없이 입력하십시오.) : ");
        scanf("%s", id);
        while (!existID(id)){
            printf("존재하지 않는 ID입니다. 다시 입력해주십시오 : ");
            scanf("%s", id);
        }
        answerAccount = getAccount(id);

        printf("비밀번호를 입력해주십시오(띄어쓰기 없이 입력하십시오.) : ");
        scanf("%s", pw);
        HashedPassword = hashPassword(pw);
        if (isPasswordSame(HashedPassword, answerAccount->pw)){
            break;    
        }
        printPassword(HashedPassword);
        printPassword(answerAccount->pw);
        printf("%d\n", isPasswordSame(HashedPassword, answerAccount->pw));
        printf("비밀번호가 일치하지 않습니다. 다시 입력하여주십시오.\n");
    }
    printf("로그인 되었습니다!\n현재 사용자 :\n");
    printPeople(answerAccount);
    return answerAccount;
}