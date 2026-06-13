#include "login.h"
#include "people.h"

const char *DATA_PATH_PEOPLE = "./database/peoples/data.jsonl";
const char *DATA_PATH_HASH = "./database/peoples/hashTable.dat";

IdHash HashTable[HASH_SIZE];

void login_create_empty_hashtable() {
	FILE *fp = fopen(DATA_PATH_HASH, "wb");

	if (fp == NULL)
		return;

	IdHash empty = login_create_empty_hash();

	for (int i = 0; i < HASH_SIZE; i++)
		fwrite(&empty, sizeof(IdHash), 1, fp);

	fclose(fp);
}

void login_save_hashtable() {
	FILE *fp = fopen(DATA_PATH_HASH, "wb");
	if (fp == NULL) {
		printf("[ERROR] file : login.c, function : login_save_hashtable.       "
			   "Can't open hash file.\n");
		return;
	}
	fwrite(HashTable, sizeof(IdHash), HASH_SIZE, fp);
	fclose(fp);
}

void login_update_hashtable(int idx) {
	FILE *fp = fopen(DATA_PATH_HASH, "rb+");
	if (fp == NULL)
		return;

	fseek(fp, idx * sizeof(IdHash), SEEK_SET);
	fwrite(&HashTable[idx], sizeof(IdHash), 1, fp);

	fclose(fp);
}

int login_load_hashtable() {
	FILE *fp = fopen(DATA_PATH_HASH, "rb");

	if (fp == NULL)
		return 0;

	size_t count = fread(HashTable, sizeof(IdHash), HASH_SIZE, fp);

	fclose(fp);

	return count == HASH_SIZE;
}
// @brief newTable에 hash를 삽입함 (open addressing, login_add_hash_to_hashtable과 동일한 방식)
//
// HashTable(전역)이 아닌 newTable 기준으로 빈 슬롯을 찾아 삽입한다.
// id는 hash1/hash2 계산이 아니라 probe 시작 위치(h1, h2) 계산에만 사용된다.
static void login_insert_into_table(IdHash *newTable, IdHash hash, char id[]) {
	ll h1 = login_hash_string(id, PRIME_HASHING1, EXPON);
	ll h2 = login_hash_string(id, PRIME_HASHING2, EXPON);
 
	int idx = h1;
	int cnt = 0;
	while (cnt++ <= HASH_SIZE) {
		if (login_is_hash_empty(newTable[idx])) {
			newTable[idx] = hash;
			return;
		}
		idx = (idx + h2) % HASH_SIZE;
	}
	printf("[ERROR] file : login.c, function : login_insert_into_table.     "
		   "Saving failed. HashTable is full(almost, probably.). Check The "
		   "Hash Table.\n");
}
 
void login_compact_database() {
	const char *tmp = "./database/peoples/data.tmp";
 
	// 임시 파일 비우기
	FILE *fp = fopen(tmp, "w");
	if (fp == NULL)
		return;
	fclose(fp);
 
	// 새 해시테이블을 힙에 할당하고 모두 빈 슬롯으로 초기화
	IdHash *newTable = (IdHash *)malloc(sizeof(IdHash) * HASH_SIZE);
	if (newTable == NULL) {
		printf("[ERROR] file : login.c, function : login_compact_database.     "
			   "Can't malloc new hash table.\n");
		remove(tmp);
		return;
	}
	for (int i = 0; i < HASH_SIZE; i++)
		newTable[i] = login_create_empty_hash();
 
	// 기존 해시테이블이 가리키는 살아있는 레코드만 한 명씩 임시 파일로 옮기고,
	// 새 해시테이블에 처음 가입하는 것처럼 다시 삽입한다.
	// (probe 체인이 끊기지 않도록 테이블 전체를 재구성)
	for (int i = 0; i < HASH_SIZE; i++) {
		if (login_is_hash_empty(HashTable[i]) || HashTable[i].offset < 0)
			continue;
 
		People *p = people_read_people(DATA_PATH_PEOPLE, HashTable[i].offset);
		// 읽기 실패(깨진 레코드/잘못된 offset) 시, 해당 레코드는 그냥 버린다.
		// newTable에 추가하지 않으므로 결과적으로 없는 계정으로 처리된다.
		if (p == NULL)
			continue;
 
		int off = people_save_people(p, tmp);
		if (off >= 0) {
			IdHash hash = login_hash_ID(p->id);
			hash.offset = off;
			login_insert_into_table(newTable, hash, p->id);
		}
		// off < 0 (새 파일 기록 실패)인 경우도 newTable에 추가하지 않고 버린다.
 
		people_delete_people(p);
	}
 
	// 새 해시테이블로 교체
	memcpy(HashTable, newTable, sizeof(IdHash) * HASH_SIZE);
	free(newTable);
 
	// data.jsonl을 압축본으로 교체하고 해시테이블 저장
	remove(DATA_PATH_PEOPLE);
	rename(tmp, DATA_PATH_PEOPLE);
	login_save_hashtable();
}

void login_init() {
	if (login_load_hashtable()) {
		login_compact_database(); // 시작 시 죽은 줄 정리
		return;
	}

	for (int i = 0; i < HASH_SIZE; i++)
		HashTable[i] = login_create_empty_hash();

	login_create_empty_hashtable();
}

IdHash login_create_empty_hash() {
	IdHash result;
	result.hash1 = -1;
	result.hash2 = -1;
	result.offset = -1;
	return result;
}

int login_is_hash_empty(IdHash hash) {
	return (hash.hash1 == -1 && hash.hash2 == -1);
}

int login_is_hash_same(IdHash hash1, IdHash hash2) {
	return (hash1.hash1 == hash2.hash1 && hash1.hash2 == hash2.hash2);
}

ll login_hash_string(char id[], ll base, ll expon) {
	ll res = 0;
	for (char *ptr = id; *ptr != '\0'; ptr++) {
		res = (res * expon + *ptr) % base;
	}
	return res;
}

IdHash login_hash_ID(char id[]) {
	IdHash result_hash;
	result_hash.hash1 = login_hash_string(id, PRIME_ENCODING1, EXPON);
	result_hash.hash2 = login_hash_string(id, PRIME_ENCODING2, EXPON);
	return result_hash;
}

int login_get_account_offset(char id[]) {
	IdHash hashed_id = login_hash_ID(id);
	ll h1 = login_hash_string(id, PRIME_HASHING1, EXPON);
	ll h2 = login_hash_string(id, PRIME_HASHING2, EXPON);

	int idx = h1;
	int cnt = 0;
	while (cnt++ <= HASH_SIZE) {
		if (login_is_hash_empty(HashTable[idx]))
			return -1;
		if (login_is_hash_same(HashTable[idx], hashed_id))
			return HashTable[idx].offset;
		idx = (idx + h2) % HASH_SIZE;
	}
	printf("[ERROR] file : login.c, function : login_get_account_offset.     "
		   "HashTable is full(almost, probably.). Check The Hash Table.\n");
	return -1;
}

int login_does_ID_exist(char id[]) {
	return login_get_account_offset(id) != -1;
}

People *login_get_account(char id[]) {
	int offset = login_get_account_offset(id);
	if (offset == -1) {
		printf("[WARNING] file : login.c, function : login_get_accunt. No "
			   "matching id found");
		return NULL;
	}
	return people_read_people(DATA_PATH_PEOPLE, offset);
}

void login_add_hash_to_hashtable(IdHash hash, char id[]) {
	ll h1 = login_hash_string(id, PRIME_HASHING1, EXPON);
	ll h2 = login_hash_string(id, PRIME_HASHING2, EXPON);

	int idx = h1;
	int cnt = 0;
	while (cnt++ <= HASH_SIZE) {
		if (login_is_hash_empty(HashTable[idx])) {
			HashTable[idx] = hash;
			login_update_hashtable(idx);
			return;
		}
		idx = (idx + h2) % HASH_SIZE;
	}
	printf("[ERROR] file : login.c, function : login_add_hash_to_hashtable.    "
		   " Saving failed. HashTable is full(almost, probably.). Check The "
		   "Hash Table.\n");
}

void login_add_people_to_hashtable(People *P) {
	int offset = people_save_people(P, DATA_PATH_PEOPLE);
	IdHash hash = login_hash_ID(P->id);
	hash.offset = offset;
	login_add_hash_to_hashtable(hash, P->id);
}

int login_update_account(People *P) {
	if (P == NULL) {
		return 0;
	}

	IdHash hashed_id = login_hash_ID(P->id);
	ll h1 = login_hash_string(P->id, PRIME_HASHING1, EXPON);
	ll h2 = login_hash_string(P->id, PRIME_HASHING2, EXPON);

	int idx = h1;
	int cnt = 0;
	while (cnt++ <= HASH_SIZE) {
		if (login_is_hash_empty(HashTable[idx])) {
			return 0;
		}
		if (login_is_hash_same(HashTable[idx], hashed_id)) {
			int offset = people_save_people(P, DATA_PATH_PEOPLE);
			if (offset < 0) {
				return 0;
			}
			HashTable[idx].offset = offset;
			login_update_hashtable(idx);
			return 1;
		}
		idx = (idx + h2) % HASH_SIZE;
	}
	return 0;
}

People *login_sign_in_account() {
	char name[MAX_NAME_LEN];
	char id[MAX_ID_LEN];
	char pw[MAX_PW_LEN];

	char type[MAX_TYPE_LEN];
	char love_type[MAX_TYPE_LEN];
	enum Gender gen;
	int age;

	printf("ID를 입력해주십시오(띄어쓰기 없이 입력하십시오.) : ");
	scanf("%s", id);
	while (login_does_ID_exist(id)) {
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

	People *resultPeople =
		people_create_people(name, id, pw, type, love_type, gen, age);
	login_add_people_to_hashtable(resultPeople);
	return resultPeople;
}

People *login_log_in_account() {
	char id[MAX_ID_LEN];
	char pw[MAX_PW_LEN];
	Password HashedPassword;
	People *answerAccount;

	while (1) {
		printf("ID를 입력해주십시오(띄어쓰기 없이 입력하십시오.) : ");
		scanf("%s", id);
		while (!login_does_ID_exist(id)) {
			printf("존재하지 않는 ID입니다. 다시 입력해주십시오 : ");
			scanf("%s", id);
		}
		answerAccount = login_get_account(id);

		printf("비밀번호를 입력해주십시오(띄어쓰기 없이 입력하십시오.) : ");
		scanf("%s", pw);
		HashedPassword = people_hash_password(pw);
		if (people_is_same_password(HashedPassword, answerAccount->pw)) {
			break;
		}
		printf("비밀번호가 일치하지 않습니다. 다시 입력하여주십시오.\n");
	}
	printf("로그인 되었습니다!\n현재 사용자 :\n");
	people_print_people(answerAccount);
	return answerAccount;
}
