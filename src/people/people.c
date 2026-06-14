#include "people.h"
#include "../headers.h"

extern const char *DATA_PATH_PEOPLE;
extern const char *DATA_PATH_HASH;

uint32_t k[64] = {
	0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a,
	0xa8304613, 0xfd469501, 0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
	0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821, 0xf61e2562, 0xc040b340,
	0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
	0x21e1cde6, 0xc33707d6, 0xfd52c019, 0xb0e9e151, 0xf98fa2cf, 0x163d6e63,
	0xb321273b, 0x32e8111f, 0xea858fa7, 0xd4ef3085, 0x04881d05, 0xd9d4d039,
	0xe6db99e5, 0x1fa27cf8, 0xc4ac5665, 0xf4292244, 0x432aff97, 0xab9423a7,
	0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1, 0x6fa87e4f,
	0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb,
	0xeb86d391, 0xb385c12d, 0xf30b2aa3, 0x8751e7d1, 0xd582a85e, 0x08a10328,
	0xec8d3b2f, 0x8b5092cf, 0xf8e3d119, 0x760d29d3};

uint32_t s[64] = {7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
				  5, 9,	 14, 20, 5, 9,	14, 20, 5, 9,  14, 20, 5, 9,  14, 20,
				  4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
				  6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};

void people_print_password(Password pw) {
	for (int i = 0; i < 16; i++) {
		printf("%02x", pw.digest[i]);
	}
	printf("\n");
}

Password people_hash_password(char pw[]) {
	uint32_t h0 = 0x67452301;
	uint32_t h1 = 0xEFCDAB89;
	uint32_t h2 = 0x98BADCFE;
	uint32_t h3 = 0x10325476;

	Password newPw;
	size_t initial_len = strlen(pw);
	size_t new_len = ((initial_len + 8) / 64 + 1) * 64;
	uint8_t *msg = (uint8_t *)calloc(new_len, 1);
	if (msg == NULL) {
		printf("[ERROR] file : people.c, function : hashPasword.     Can't "
			   "calloc structure.\n");
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
	memcpy(newPw.digest + 8, &h2, 4);
	memcpy(newPw.digest + 12, &h3, 4);

	free(msg);
	return newPw;
}

Password people_read_password_from_hex(char pw_hex[]) {
	Password pw;
	for (int i = 0; i < 16; i++) {
		sscanf(&pw_hex[i * 2], "%2hhx", &pw.digest[i]);
	}
	return pw;
}

int people_is_same_password(Password p1, Password p2) {
	return memcmp(p1.digest, p2.digest, 16) == 0;
}

People* people_create_people(
    char name[],
    char id[],
    char pw[],
    char type[],
    char love_type[],
    char attach[],
    char love_attach[],
    char lang[],
    char love_lang[],
    enum Gender gen,
    int age
){
    People* newPeople = (People*)malloc(sizeof(People));
    if (newPeople == NULL){
        printf("[ERROR] file : people.c, function : people_create_people.     Can't malloc structure.\n");
        return NULL;
    }

	strncpy(newPeople->name, name, MAX_NAME_LEN - 1);
	strncpy(newPeople->id, id, MAX_ID_LEN - 1);
	newPeople->lover[0] = '\0';
	newPeople->pw = people_hash_password(pw);
	strncpy(newPeople->type, type, MAX_TYPE_LEN - 1);
	strncpy(newPeople->love_type, love_type, MAX_TYPE_LEN - 1);
	strncpy(newPeople->attach, attach, MAX_TYPE_LEN - 1);
	strncpy(newPeople->love_attach, love_attach, MAX_TYPE_LEN - 1);
	strncpy(newPeople->lang, lang, MAX_TYPE_LEN - 1);
	strncpy(newPeople->love_lang, love_lang, MAX_TYPE_LEN - 1);
	newPeople->name[MAX_NAME_LEN - 1] = '\0';
	newPeople->id[MAX_ID_LEN - 1] = '\0';
	newPeople->type[MAX_TYPE_LEN - 1] = '\0';
	newPeople->love_type[MAX_TYPE_LEN - 1] = '\0';
	newPeople->attach[MAX_TYPE_LEN - 1] = '\0';
	newPeople->love_attach[MAX_TYPE_LEN - 1] = '\0';
	newPeople->lang[MAX_TYPE_LEN - 1] = '\0';
	newPeople->love_lang[MAX_TYPE_LEN - 1] = '\0';
	newPeople->gen = gen;
	newPeople->age = age;
	newPeople->status = UNSWORDMASTER; // 기본 매칭 가능 상태 (팀원 변경 반영)
	newPeople->blocked_cnt = 0;
	newPeople->dfs_extended = 0; // 세분화 1회 제한 플래그(세션 전용)
	return newPeople;
}

void people_set_people_name(People *P, char name[]) {
	strncpy(P->name, name, MAX_NAME_LEN - 1);
	P->name[MAX_NAME_LEN - 1] = '\0';
}

void people_set_people_type(People *P, char type[]) {
	strncpy(P->type, type, MAX_TYPE_LEN - 1);
	P->type[MAX_TYPE_LEN - 1] = '\0';
}

void people_set_people_love_type(People *P, char love_type[]) {
	strncpy(P->love_type, love_type, MAX_TYPE_LEN - 1);
	P->love_type[MAX_TYPE_LEN - 1] = '\0';
}

void people_set_people_attach(People *P, char attach[]) {
	strncpy(P->attach, attach, MAX_TYPE_LEN - 1);
	P->attach[MAX_TYPE_LEN - 1] = '\0';
}

void people_set_people_love_attach(People *P, char love_attach[]) {
	strncpy(P->love_attach, love_attach, MAX_TYPE_LEN - 1);
	P->love_attach[MAX_TYPE_LEN - 1] = '\0';
}

void people_set_people_lang(People *P, char lang[]) {
	strncpy(P->lang, lang, MAX_TYPE_LEN - 1);
	P->lang[MAX_TYPE_LEN - 1] = '\0';
}

void people_set_people_love_lang(People *P, char love_lang[]) {
	strncpy(P->love_lang, love_lang, MAX_TYPE_LEN - 1);
	P->love_lang[MAX_TYPE_LEN - 1] = '\0';
}

void people_set_people_id(People *P, char id[]) {
	strncpy(P->id, id, MAX_ID_LEN - 1);
	P->id[MAX_ID_LEN - 1] = '\0';
}

void people_set_people_lover(People *P, char lover[]) {
	strncpy(P->lover, lover, MAX_ID_LEN - 1);
	P->lover[MAX_ID_LEN - 1] = '\0';
}

void people_set_people_pw(People *P, char pw[]) {
	P->pw = people_hash_password(pw);
}

void people_set_people_status(People *P, MatchStatus status) {
	P->status = status;
}

void people_set_people_pw_hashed(People *P, Password pw) { P->pw = pw; }

void people_set_people_gen( People* P, enum Gender gen){
    P->gen = gen;
}

void people_set_people_age( People* P, int age){
    P->age = age;
}

void people_set_people_block(People* P, int blocked_cnt, char blocked_ids[][MAX_ID_LEN]){
    P->blocked_cnt = blocked_cnt;
    for (int i = 0 ; i < blocked_cnt ; i++){
        strcpy(P->blocked_ids[i], blocked_ids[i]);
    }
}

char *people_read_text_from_file(const char path[], int offset) {
	FILE *fp = fopen(path, "r");
	if (fp == NULL) {
		printf(
			"[ERROR] file : people.c, function : people_read_text_from_file.   "
			"  Can't read file. Probably Wrong Path\n");
		return NULL;
	}

	char *buffer = (char *)malloc(512);
	fseek(fp, offset, SEEK_SET);
	fgets(buffer, 512, fp);
	fclose(fp);
	return buffer;
}

People *people_read_people(const char path[], int offset) {
	char *text = people_read_text_from_file(path, offset);
	if (text == NULL) {
		printf("[ERROR] file : people.c, function : people_read_people.     "
			   "Can't read text from people_read_text_from_file. Probably "
			   "Error hold on people_read_text_from_file.\n");
		return NULL;
	}

	cJSON *root = cJSON_Parse(text);

	if (root == NULL) {
		printf("%s\n", text);
		printf("[ERROR] file : people.c, function : people_read_people.     "
			   "Can't parse text from text.\n");
		free(text);
		return NULL;
	}

	char name[MAX_NAME_LEN];
	char id[MAX_ID_LEN];
	char lover[MAX_ID_LEN];
	Password pw;
	char type[MAX_TYPE_LEN];
	char love_type[MAX_TYPE_LEN];
	strcpy(name, cJSON_GetObjectItem(root, "name")->valuestring);
	strcpy(id, cJSON_GetObjectItem(root, "id")->valuestring);
	strcpy(lover, cJSON_GetObjectItem(root, "lover")->valuestring);
	strcpy(type, cJSON_GetObjectItem(root, "type")->valuestring);
	strcpy(love_type, cJSON_GetObjectItem(root, "love_type")->valuestring);
	char *pw_hex = cJSON_GetObjectItem(root, "pw")->valuestring;
	pw = people_read_password_from_hex(pw_hex);


    enum Gender gen  = cJSON_GetObjectItem(root, "gen" )->valueint;
    int         age  = cJSON_GetObjectItem(root, "age" )->valueint;

    // 애착 유형은 구버전 데이터엔 없을 수 있으니 키가 있을 때만 설정 (없으면 "")
    cJSON* attach_item      = cJSON_GetObjectItem(root, "attach");
    cJSON* love_attach_item = cJSON_GetObjectItem(root, "love_attach");

    cJSON* lang_item      = cJSON_GetObjectItem(root, "lang");
    cJSON* love_lang_item = cJSON_GetObjectItem(root, "love_lang");

    MatchStatus status      = cJSON_GetObjectItem(root, "status")->valueint;
    int         blocked_cnt = cJSON_GetObjectItem(root, "blocked_cnt")->valueint;

    cJSON *blocked = cJSON_GetObjectItem(root, "blocked_ids");
    char blocked_ids[MAX_BLOCKED][MAX_ID_LEN];
    for (int i = 0 ; i < blocked_cnt ; i++){
        strncpy(
            blocked_ids[i],
            cJSON_GetArrayItem(blocked, i)->valuestring,
            MAX_ID_LEN - 1
        );
        blocked_ids[i][MAX_ID_LEN - 1] = '\0';
    }

    People* resultPeople = people_create_people(
        name, id, "\0",
        type, love_type, attach_item->valuestring, love_attach_item->valuestring, lang_item->valuestring, love_lang_item->valuestring,
        gen, age);
    people_set_people_pw_hashed(resultPeople, pw);
    people_set_people_status(resultPeople, status);
    people_set_people_block(resultPeople, blocked_cnt, blocked_ids);

	cJSON_Delete(root);
	free(text);

	return resultPeople;
}

int people_save_people(People *P, const char path[]) {
	cJSON *root = cJSON_CreateObject();

	cJSON_AddStringToObject(root, "name", P->name);
	cJSON_AddStringToObject(root, "id", P->id);
	cJSON_AddStringToObject(root, "lover", P->lover);

	char pw_hex[33];
	for (int i = 0; i < 16; i++)
		sprintf(&pw_hex[i * 2], "%02x", P->pw.digest[i]);

	pw_hex[32] = '\0';

    cJSON_AddStringToObject(root, "pw", pw_hex);
    cJSON_AddStringToObject(root, "type", P->type);
    cJSON_AddStringToObject(root, "love_type", P->love_type);
    cJSON_AddStringToObject(root, "attach", P->attach);
    cJSON_AddStringToObject(root, "love_attach", P->love_attach);
    cJSON_AddStringToObject(root, "lang", P->lang);
    cJSON_AddStringToObject(root, "love_lang", P->love_lang);
    cJSON_AddNumberToObject(root, "gen", P->gen);
    cJSON_AddNumberToObject(root, "age", P->age);
    cJSON_AddNumberToObject(root, "status", P->status);
    cJSON_AddNumberToObject(root, "blocked_cnt", P->blocked_cnt);

    cJSON *blocked = cJSON_CreateArray();

    for (int i = 0; i < P->blocked_cnt; i++) {
        cJSON_AddItemToArray(
            blocked,
            cJSON_CreateString(P->blocked_ids[i])
        );
    }
    cJSON_AddItemToObject(root, "blocked_ids", blocked);

	char *json_string = cJSON_PrintUnformatted(root);

	FILE *fp = fopen(path, "a");

	if (fp == NULL) {
		printf("[ERROR] Can't open file.\n");
		free(json_string);
		cJSON_Delete(root);
		return -1;
	}

	fseek(fp, 0, SEEK_END);
	int offset = ftell(fp);
	fprintf(fp, "%s\n", json_string);
	fclose(fp);
	free(json_string);
	cJSON_Delete(root);
	return offset;
}

static unsigned long people_simple_hash(const char *s) {
	unsigned long h = 5381;
	for (; *s; s++)
		h = h * 33 + (unsigned char)*s;
	return h;
}

static int people_temp_set_contains_or_add(TempIdSet *set, const char *id) {
	unsigned long h = people_simple_hash(id) % TEMP_SET_SIZE;
	for (int tries = 0; tries < TEMP_SET_SIZE; tries++) {
		int idx = (h + tries) % TEMP_SET_SIZE;
		if (!set[idx].used) {
			set[idx].used = 1;
			strncpy(set[idx].id, id, MAX_ID_LEN - 1);
			set[idx].id[MAX_ID_LEN - 1] = '\0';
			return 0; // 새로 추가됨
		}
		if (strcmp(set[idx].id, id) == 0) {
			return 1; // 이미 존재
		}
	}
	return 1; // 셋이 가득 찬 경우 (사실상 발생하지 않음)
}

People **people_read_all_people(int *count) {
	FILE *fp = fopen(DATA_PATH_PEOPLE, "r");
	if (fp == NULL) {
		printf("[ERROR] file : people.c, function : people_read_all_people.    "
			   " Can't open data file.\n");
		*count = 0;
		return NULL;
	}

	// 1. 전체 파일을 줄 단위로 메모리에 적재
	int line_capacity = 16;
	int line_cnt = 0;
	char **lines = (char **)malloc(sizeof(char *) * line_capacity);

	char buf[512];
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		if (line_cnt >= line_capacity) {
			line_capacity *= 2;
			lines = (char **)realloc(lines, sizeof(char *) * line_capacity);
		}
		lines[line_cnt] = strdup(buf);
		line_cnt++;
	}
	fclose(fp);

	// 2. 임시 해시셋 준비
	TempIdSet *seen = (TempIdSet *)calloc(TEMP_SET_SIZE, sizeof(TempIdSet));

	// 3. 결과 배열 준비
	int capacity = 16;
	int cnt = 0;
	People **result = (People **)malloc(sizeof(People *) * capacity);

	// 4. 아래(최신)부터 위(과거)로 순회
	for (int i = line_cnt - 1; i >= 0; i--) {
		cJSON *root = cJSON_Parse(lines[i]);
		if (root == NULL) {
			free(lines[i]);
			continue;
		}

		cJSON *id_item = cJSON_GetObjectItem(root, "id");
		if (!cJSON_IsString(id_item) || id_item->valuestring == NULL) {
			cJSON_Delete(root);
			free(lines[i]);
			continue;
		}

		// 이미 처리한 id면 (더 최신 데이터가 있었음) 스킵
		if (people_temp_set_contains_or_add(seen, id_item->valuestring)) {
			cJSON_Delete(root);
			free(lines[i]);
			continue;
		}

		// People 생성 (people_read_people과 동일한 파싱)
		char name[MAX_NAME_LEN];
		char id[MAX_ID_LEN];
		char lover[MAX_ID_LEN];
		char type[MAX_TYPE_LEN];
		char love_type[MAX_TYPE_LEN];

		strcpy(name, cJSON_GetObjectItem(root, "name")->valuestring);
		strcpy(id, id_item->valuestring);
		strcpy(lover, cJSON_GetObjectItem(root, "lover")->valuestring);
		strcpy(type, cJSON_GetObjectItem(root, "type")->valuestring);
		strcpy(love_type, cJSON_GetObjectItem(root, "love_type")->valuestring);

		char *pw_hex = cJSON_GetObjectItem(root, "pw")->valuestring;
		Password pw = people_read_password_from_hex(pw_hex);

        enum Gender gen = cJSON_GetObjectItem(root, "gen")->valueint;
        int age         = cJSON_GetObjectItem(root, "age")->valueint;

        cJSON* attach_item      = cJSON_GetObjectItem(root, "attach");
        cJSON* love_attach_item = cJSON_GetObjectItem(root, "love_attach");

        cJSON* lang_item      = cJSON_GetObjectItem(root, "lang");
        cJSON* love_lang_item = cJSON_GetObjectItem(root, "love_lang");

        MatchStatus status      = cJSON_GetObjectItem(root, "status")->valueint;
        int         blocked_cnt = cJSON_GetObjectItem(root, "blocked_cnt")->valueint;

        cJSON *blocked = cJSON_GetObjectItem(root, "blocked_ids");
        char blocked_ids[MAX_BLOCKED][MAX_ID_LEN];
        for (int i = 0 ; i < blocked_cnt ; i++){
            strncpy(
                blocked_ids[i],
                cJSON_GetArrayItem(blocked, i)->valuestring,
                MAX_ID_LEN - 1
            );
            blocked_ids[i][MAX_ID_LEN - 1] = '\0';
        }

        People* p = people_create_people(
            name, id, "\0",
            type, love_type, attach_item->valuestring, love_attach_item->valuestring, lang_item->valuestring, love_lang_item->valuestring,
            gen, age);
        people_set_people_pw_hashed(p, pw);
        people_set_people_status(p, status);
        people_set_people_block(p, blocked_cnt, blocked_ids);

		cJSON_Delete(root);
		free(lines[i]);

		if (cnt >= capacity) {
			capacity *= 2;
			result = (People **)realloc(result, sizeof(People *) * capacity);
		}
		result[cnt++] = p;
	}

	free(lines);
	free(seen);

	*count = cnt;
	return result;
}

void people_delete_all_people(People **people, int count) {
	for (int i = 0; i < count; i++) {
		people_delete_people(people[i]);
	}
	free(people);
}

void people_print_people(People *P) {
	if (P == NULL) {
		printf("NULL People\n");
		return;
	}

	printf("Name      : %s\n", P->name);
	printf("ID        : %s\n", P->id);

	printf("Password  : ");
	for (int i = 0; i < 16; i++) {
		printf("%02x", P->pw.digest[i]);
	}
	printf("\n");

	printf("Type      : %s\n", P->type);
	printf("Love Type : %s\n", P->love_type);

	printf("Gender    : %s\n", P->gen == GENDER_MALE ? "Male" : "Female");

	printf("Age       : %d\n", P->age);
}

void people_delete_people(People *P) { free(P); }