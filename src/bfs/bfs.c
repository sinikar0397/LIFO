#include "bfs.h"
#include "../dfs/dfs.h"
#include "../headers.h"

#define BLOCK -500000000

// compat(People *a, People *b) 함수 있어야 함!!
// dfs에서 구현 후 헤더로 불러와야 됨

int bfsCompat(People *a, People *b) {
	int block = 0;
	block += BLOCK * (isBlocked(a, b) + isBlocked(b, a));
	return compat(a, b) + block;
}

// 큐 구현
typedef struct Queue {
	int total_size;
	int data[MAX_PEOPLE + 5];
	int front;
	int rear;
} Queue;

Queue *createQueue() {
	Queue *q = (Queue *)malloc(sizeof(Queue));
	if (q == NULL)
		return NULL;
	q->front = 0;
	q->rear = 0;
	q->total_size = MAX_PEOPLE + 5;
	return q;
}

int isQueueEmpty(Queue *q) { return q->front == q->rear; }

int isQueueFull(Queue *q) { return (q->rear + 1) % q->total_size == q->front; }

void pushQueue(Queue *q, int x) {
	if (q == NULL)
		return;
	if (isQueueFull(q)) {
		printf("Error: Queue is Full\n");
		return;
	}
	q->data[q->rear] = x;
	q->rear = (q->rear + 1) % q->total_size;
}

int popQueue(Queue *q) {
	if (q == NULL)
		return -1;
	if (isQueueEmpty(q)) {
		printf("Error: Queue is Empty\n");
		return -1;
	}
	int x = q->data[q->front];
	q->front = (q->front + 1) % q->total_size;
	return x;
}

void initMatchingInfo(MatchingInfo *info, People *person) {
	if (info == NULL)
		return;
	info->person = person;
	info->match_idx = -1;
	info->pref_size = 0;
	info->next_proposal = 0;

	for (int i = 0; i < MAX_PEOPLE; i++) {
		info->preference[i] = -1;
	}
}

void initMatchingInfos(MatchingInfo infos[], People *people[], int n) {
	if (infos == NULL || people == NULL)
		return;
	for (int i = 0; i < n; i++) {
		initMatchingInfo(&infos[i], people[i]);
	}
}

void changeStatus(MatchingInfo infos[], int idx, MatchStatus status) {
	if (infos == NULL || idx < 0)
		return;
	infos[idx].person->status = status;
}

void addUser(MatchingInfo infos[], int *n, People *new_person) {
	if (infos == NULL || n == NULL || new_person == NULL)
		return;
	if (*n >= MAX_PEOPLE) {
		printf("Error: Too many people\n");
		return;
	}
	initMatchingInfo(&infos[*n], new_person);
	(*n)++;
}

void removeUser(MatchingInfo infos[], int user_idx) {
	if (infos == NULL || user_idx < 0)
		return;

	int partner_idx = infos[user_idx].match_idx;
	if (partner_idx != -1) {
		infos[partner_idx].match_idx = -1;
		if (infos[partner_idx].person->status != DELETED) {
			infos[partner_idx].person->status = PAUSED;
		}
	}

	infos[user_idx].match_idx = -1;
	infos[user_idx].person->status = DELETED;
	infos[user_idx].pref_size = 0;
	infos[user_idx].next_proposal = 0;
}

int collectUser(MatchingInfo infos[], int n, enum Gender gen, int result[]) {
	if (infos == NULL || result == NULL || n <= 0)
		return 0;

	int cnt = 0;
	for (int i = 0; i < n; i++) {
		if (infos[i].person == NULL)
			continue;
		if (infos[i].person->status != AVAILABLE)
			continue;
		if (infos[i].person->gen != gen)
			continue;
		result[cnt++] = i;
	}

	return cnt;
}

int compareCandidate(const void *a, const void *b) {
	const Candidate *x = (const Candidate *)a;
	const Candidate *y = (const Candidate *)b;

	if (x->score != y->score)
		return y->score - x->score;
	return x->idx - y->idx;
}

void makePrefList(MatchingInfo infos[], int user_idx, int candidates[],
				  int cand_size) {
	if (infos == NULL || candidates == NULL)
		return;
	if (user_idx < 0 || user_idx >= MAX_PEOPLE)
		return;
	if (infos[user_idx].person == NULL)
		return;

	Candidate tmp[MAX_PEOPLE];
	int tmp_size = 0;
	for (int i = 0; i < cand_size; i++) {
		int cand_idx = candidates[i];

		if (cand_idx < 0 || cand_idx >= MAX_PEOPLE)
			continue;
		if (infos[cand_idx].person == NULL)
			continue;
		if (infos[cand_idx].person->status != AVAILABLE)
			continue;

		tmp[tmp_size].idx = cand_idx;
		tmp[tmp_size].score =
			bfsCompat(infos[user_idx].person, infos[cand_idx].person);
		tmp_size++;
	}

	// candidate compatibility 순 정렬
	qsort(tmp, tmp_size, sizeof(Candidate), compareCandidate);

	infos[user_idx].pref_size = tmp_size;
	infos[user_idx].next_proposal = 0;

	for (int i = 0; i < MAX_PEOPLE; i++) {
		infos[user_idx].preference[i] = -1;
	}
	for (int i = 0; i < tmp_size; i++) {
		infos[user_idx].preference[i] = tmp[i].idx;
	}
}

void makePrefLists(MatchingInfo infos[], int n) {
	if (infos == NULL || n <= 0)
		return;

	int males[MAX_PEOPLE];
	int females[MAX_PEOPLE];

	int male_cnt = collectUser(infos, n, GENDER_MALE, males);
	int female_cnt = collectUser(infos, n, GENDER_FEMALE, females);

	for (int i = 0; i < male_cnt; i++) {
		int user_idx = males[i];
		makePrefList(infos, user_idx, females, female_cnt);
	}
	for (int i = 0; i < female_cnt; i++) {
		int user_idx = females[i];
		makePrefList(infos, user_idx, males, male_cnt);
	}
}

void createProposal(MatchingInfo infos[], int u, int v) {
	if (infos == NULL || u < 0 || v < 0)
		return;

	infos[u].match_idx = v;
	infos[v].match_idx = u;
	infos[u].person->status = PROPOSED;
	infos[v].person->status = PROPOSED;
}

void acceptMatch(MatchingInfo infos[], int u, int v) {
	if (infos == NULL || u < 0 || v < 0)
		return;

	if (infos[u].match_idx = v && infos[v].match_idx == u) {
		if (infos[v].person->status == ACCEPTED) {
			infos[u].person->status = MATCHED;
			infos[v].person->status = MATCHED;
		}
		else if (infos[v].person->status == PROPOSED) {
			infos[u].person->status = ACCEPTED;
		}
		else {
			printf("Error: User %d is not PROPOSED or ACCEPTED\n", v);
		}
	}
	else {
		printf("서로 PROPOSED되지 않은 상대끼리 acceptMatch를 시도했습니다. \n");
		return;
	}
}

void rejectMatch(MatchingInfo infos[], int u, int v) {
	if (infos == NULL || u < 0 || v < 0)
		return;

	infos[u].match_idx = -1;
	infos[v].match_idx = -1;

	if (infos[u].person->status != DELETED) {
		infos[u].person->status = AVAILABLE;
	}
	if (infos[v].person->status != DELETED) {
		infos[v].person->status = AVAILABLE;
	}
}

int stableMatching(MatchingInfo infos[], int n, int proposers[],
				   int proposer_cnt, int rank_table[][MAX_PEOPLE],
				   Pair result[]) {
	if (infos == NULL || proposers == NULL || rank_table == NULL ||
		result == NULL)
		return 0;
	if (n <= 0 || proposer_cnt <= 0)
		return 0;

	// AVAILABLE 사용자 상태 초기화
	for (int i = 0; i < n; i++) {
		if (infos[i].person->status == AVAILABLE) {
			infos[i].match_idx = -1;
			infos[i].next_proposal = 0;
		}
	}

	// 선호도 리스트 생성
	makePrefLists(infos, n);

	// rank_table 초기화
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			rank_table[i][j] = MAX_PEOPLE + 1;
		}
	}

	// rank table 생성
	// rank_table[i][j]: i가 j를 몇 번째로 선호하는가?
	for (int i = 0; i < n; i++) {
		if (infos[i].person->status != AVAILABLE) {
			continue;
		}

		for (int rank = 0; rank < infos[i].pref_size; rank++) {
			int target_idx = infos[i].preference[rank];
			if (target_idx < 0 || target_idx >= n)
				continue;
			rank_table[i][target_idx] = rank;
		}
	}

	// 미매칭 proposer 큐에 넣기
	Queue *q = createQueue();
	if (q == NULL) {
		printf("malloc failed\n");
		return 0;
	}
	for (int i = 0; i < proposer_cnt; i++) {
		int proposer_idx = proposers[i];

		if (proposer_idx < 0 || proposer_idx >= n)
			continue;
		if (infos[proposer_idx].person == NULL)
			continue;
		if (infos[proposer_idx].person->status != AVAILABLE)
			continue;

		pushQueue(q, proposer_idx);
	}

	// Gale-Shapley Stable Matching
	while (!isQueueEmpty(q)) {
		int proposer_idx = popQueue(q);
		if (proposer_idx < 0 || proposer_idx >= n)
			continue;
		if (infos[proposer_idx].person->status != AVAILABLE)
			continue;
		if (infos[proposer_idx].match_idx != -1)
			continue;

		while (infos[proposer_idx].next_proposal <
			   infos[proposer_idx].pref_size) {
			int receiver_idx =
				infos[proposer_idx]
					.preference[infos[proposer_idx].next_proposal];
			infos[proposer_idx].next_proposal++;
			if (receiver_idx < 0 || receiver_idx >= n)
				continue;
			if (infos[receiver_idx].person->status != AVAILABLE)
				continue;

			if (infos[receiver_idx].match_idx == -1) {
				infos[proposer_idx].match_idx = receiver_idx;
				infos[receiver_idx].match_idx = proposer_idx;
				break;
			}

			int old_proposer_idx = infos[receiver_idx].match_idx;
			if (rank_table[receiver_idx][proposer_idx] <
				rank_table[receiver_idx][old_proposer_idx]) {

				/* 기존 상대 out */
				infos[old_proposer_idx].match_idx = -1;

				/* 새 proposer와 receiver 매칭 */
				infos[proposer_idx].match_idx = receiver_idx;
				infos[receiver_idx].match_idx = proposer_idx;

				pushQueue(q, old_proposer_idx);
				break;
			}
		}
	}

	free(q);

	int result_cnt = 0;
	for (int i = 0; i < proposer_cnt; i++) {
		int proposer_idx = proposers[i];
		if (proposer_idx < 0 || proposer_idx >= n)
			continue;
		int receiver_idx = infos[proposer_idx].match_idx;
		if (receiver_idx == -1)
			continue;

		result[result_cnt].p1 = proposer_idx;
		result[result_cnt].p2 = receiver_idx;
		result_cnt++;
	}

	for (int i = 0; i < result_cnt; i++) {
		createProposal(infos, result[i].p1, result[i].p2);
	}
	return result_cnt;
}

int isBlocked(People *a, People *b) {
	for (int i = 0; i < a->blocked_cnt; i++) {
		if (strcmp(a->blocked_ids[i], b->id) == 0) {
			return 1;
		}
	}
	return 0;
}

void blockUser(People *a, People *b) {
	if (a->blocked_cnt >= MAX_BLOCKED) {
		printf("최대 차단 인원 %d명 초과\n", MAX_BLOCKED);
		return;
	}

	// 이미 차단된 경우
	if (isBlocked(a, b)) {
		return;
	}

	strcpy(a->blocked_ids[a->blocked_cnt], b->id);
	a->blocked_cnt++;
	return;
}

void unblockUser(People *a, People *b) {
	for (int i = 0; i < a->blocked_cnt; i++) {
		if (strcmp(a->blocked_ids[i], b->id) == 0) {
			if (i != a->blocked_cnt - 1) {
				strcpy(a->blocked_ids[i], a->blocked_ids[a->blocked_cnt - 1]);
			}
			a->blocked_cnt--;
			return;
		}
	}
}

int bfs_loadPeopleFromDatabase(People *people[]) {
	FILE *fp = fopen(PEOPLE_DATA_PATH, "r");
	if (fp == NULL) {
		printf("파일 열기 실패: 경로 이슈\n");
		return 0;
	}
	char line[1024];
	int n = 0;

	while (n < MAX_PEOPLE) {
		int offset = ftell(fp);
		if (fgets(line, sizeof(line), fp) == NULL) {
			break;
		}
		if (line[0] == '\n' || line[0] == '\0') {
			continue;
		}
		people[n] = people_read_people(PEOPLE_DATA_PATH, offset);
		if (people[n] != NULL) {
			n++;
		}
	}

	fclose(fp);
	// printf("Loaded %d people from %s\n", n, PEOPLE_DATA_PATH);
	return n;
}