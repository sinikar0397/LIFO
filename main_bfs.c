#include "./src/bfs/bfs.h"
#include "./src/dfs/dfs.h"
#include "./src/headers.h"
#include "./src/people/people.h"

#define PEOPLE_DATA_PATH "./database/peoples/data.jsonl"
#define PEOPLE_DATA_PATH_FALLBACK "./LIFO/database/peoples/data.jsonl"

static void clearInputBuffer(void) {
	int ch;
	while ((ch = getchar()) != '\n' && ch != EOF) {
	}
}

static const char *genderToString(enum Gender gen) {
	return gen == GENDER_MALE ? "Male" : "Female";
}

static const char *statusToString(MatchStatus status) {
	switch (status) {
	case AVAILABLE:
		return "AVAILABLE";
	case PROPOSED:
		return "PROPOSED";
	case MATCHED:
		return "MATCHED";
	case PAUSED:
		return "PAUSED";
	case DELETED:
		return "DELETED";
	default:
		return "UNKNOWN";
	}
}

static FILE *openPeopleDataFile(const char **path) {
	FILE *fp = fopen(PEOPLE_DATA_PATH, "r");
	if (fp != NULL) {
		*path = PEOPLE_DATA_PATH;
		return fp;
	}

	fp = fopen(PEOPLE_DATA_PATH_FALLBACK, "r");
	if (fp != NULL) {
		*path = PEOPLE_DATA_PATH_FALLBACK;
		return fp;
	}

	*path = NULL;
	return NULL;
}

static void freePeopleList(People *people[], int n) {
	for (int i = 0; i < n; i++) {
		people_delete_people(people[i]);
		people[i] = NULL;
	}
}

static int loadPeopleFromDatabase(People *people[]) {
	const char *path = NULL;
	FILE *fp = openPeopleDataFile(&path);
	char line[1024];
	int n = 0;

	if (fp == NULL) {
		printf("Cannot open people database file.\n");
		printf("Tried: %s\n", PEOPLE_DATA_PATH);
		printf("Tried: %s\n", PEOPLE_DATA_PATH_FALLBACK);
		return 0;
	}

	while (n < MAX_PEOPLE) {
		int offset = ftell(fp);
		if (fgets(line, sizeof(line), fp) == NULL) {
			break;
		}
		if (line[0] == '\n' || line[0] == '\0') {
			continue;
		}

		people[n] = people_read_people(path, offset);
		if (people[n] != NULL) {
			n++;
		}
	}

	fclose(fp);
	printf("Loaded %d people from %s\n", n, path);
	return n;
}

static void printUsers(MatchingInfo infos[], int n) {
	if (n == 0) {
		printf("No users.\n");
		return;
	}

	printf("\n%-4s %-12s %-12s %-8s %-10s %-10s %-5s %-10s %-12s\n",
		   "idx", "id", "name", "gender", "type", "love_type", "age", "status", "match");

	for (int i = 0; i < n; i++) {
		People *p = infos[i].person;
		if (p == NULL) {
			continue;
		}

		char match_id[MAX_ID_LEN] = "-";
		if (infos[i].match_idx >= 0 && infos[i].match_idx < n &&
			infos[infos[i].match_idx].person != NULL) {
			strncpy(match_id, infos[infos[i].match_idx].person->id, MAX_ID_LEN - 1);
			match_id[MAX_ID_LEN - 1] = '\0';
		}

		printf("%-4d %-12s %-12s %-8s %-10s %-10s %-5d %-10s %-12s\n",
			   i,
			   p->id,
			   p->name,
			   genderToString(p->gen),
			   p->type,
			   p->love_type,
			   p->age,
			   statusToString(infos[i].status),
			   match_id);
	}
}

static int findUserById(MatchingInfo infos[], int n, char id[]) {
	for (int i = 0; i < n; i++) {
		if (infos[i].person != NULL && strcmp(infos[i].person->id, id) == 0) {
			return i;
		}
	}
	return -1;
}

static int chooseCurrentUser(MatchingInfo infos[], int n) {
	char id[MAX_ID_LEN];

	printf("Your id: ");
	scanf("%19s", id);

	int idx = findUserById(infos, n, id);
	if (idx == -1) {
		printf("Cannot find id '%s' in database.\n", id);
		return -1;
	}

	printf("Current user: %s (%s)\n", infos[idx].person->name, infos[idx].person->id);
	return idx;
}

static void resetProposals(MatchingInfo infos[], int n) {
	for (int i = 0; i < n; i++) {
		if (infos[i].status == PROPOSED) {
			infos[i].status = AVAILABLE;
			infos[i].match_idx = -1;
			infos[i].next_proposal = 0;
		}
	}
}

static int runStableMatching(MatchingInfo infos[], int n, Pair result[]) {
	if (n <= 0) {
		printf("No users to match.\n");
		return 0;
	}

	resetProposals(infos, n);

	int proposers[MAX_PEOPLE];
	int proposer_cnt = collectUser(infos, n, GENDER_MALE, proposers);
	if (proposer_cnt == 0) {
		printf("No available male proposers.\n");
		return 0;
	}

	int (*rank_table)[MAX_PEOPLE] = calloc((size_t)n, sizeof(*rank_table));
	if (rank_table == NULL) {
		printf("Failed to allocate rank table.\n");
		return 0;
	}

	int result_cnt = stableMatching(infos, n, proposers, proposer_cnt, rank_table, result);
	free(rank_table);

	printf("\nRecommended pairs: %d\n", result_cnt);
	for (int i = 0; i < result_cnt; i++) {
		int p1 = result[i].p1;
		int p2 = result[i].p2;

		printf("[%d] %s(%s) - %s(%s), score: %d\n",
			   i,
			   infos[p1].person->name,
			   infos[p1].person->id,
			   infos[p2].person->name,
			   infos[p2].person->id,
			   compat(infos[p1].person, infos[p2].person));
	}

	if (result_cnt == 0) {
		printf("No match was created. Check whether both male and female AVAILABLE users exist.\n");
	}

	return result_cnt;
}

static int findProposalForUser(MatchingInfo infos[], Pair result[], int result_cnt,
							   int user_idx) {
	for (int i = 0; i < result_cnt; i++) {
		int p1 = result[i].p1;
		int p2 = result[i].p2;

		if (p1 != user_idx && p2 != user_idx) {
			continue;
		}
		if (infos[p1].status == PROPOSED && infos[p2].status == PROPOSED) {
			return i;
		}
	}

	return -1;
}

static void respondToMyProposal(MatchingInfo infos[], int n, Pair result[],
								int result_cnt, int current_idx) {
	if (current_idx < 0 || current_idx >= n) {
		printf("Choose your id first.\n");
		return;
	}

	int proposal_idx = findProposalForUser(infos, result, result_cnt, current_idx);
	if (proposal_idx == -1) {
		printf("No pending proposal for %s.\n", infos[current_idx].person->id);
		return;
	}

	int p1 = result[proposal_idx].p1;
	int p2 = result[proposal_idx].p2;
	int partner_idx = p1 == current_idx ? p2 : p1;
	char answer;

	printf("\nYour proposal:\n");
	printf("%s(%s) - %s(%s), score: %d\n",
		   infos[p1].person->name,
		   infos[p1].person->id,
		   infos[p2].person->name,
		   infos[p2].person->id,
		   compat(infos[p1].person, infos[p2].person));
	printf("Accept this match with %s? (y/n): ", infos[partner_idx].person->name);

	scanf(" %c", &answer);
	if (answer == 'y' || answer == 'Y') {
		confirmMatch(infos, p1, p2);
		printf("Match confirmed.\n");
	} else {
		rejectMatch(infos, p1, p2);
		printf("Match rejected.\n");
	}
}

int main(void) {
	People *people[MAX_PEOPLE] = {0};
	MatchingInfo *infos = calloc(MAX_PEOPLE, sizeof(MatchingInfo));
	Pair *result = calloc(MAX_PEOPLE, sizeof(Pair));
	int n = 0;
	int result_cnt = 0;
	int current_idx = -1;

	if (infos == NULL || result == NULL) {
		printf("Failed to allocate memory.\n");
		free(infos);
		free(result);
		return 1;
	}

	n = loadPeopleFromDatabase(people);
	initMatchingInfos(infos, people, n);

	while (1) {
		int cmd;

		printf("\n");
		printf("========================================\n");
		printf(" BFS Stable Matching Main\n");
		printf("========================================\n");
		printf("1. Reload people database\n");
		printf("2. Print users\n");
		printf("3. Choose my id\n");
		printf("4. Run stable matching\n");
		printf("5. Accept/reject my proposal\n");
		printf("6. Exit\n");
		printf("input: ");

		if (scanf("%d", &cmd) != 1) {
			clearInputBuffer();
			printf("Invalid input.\n");
			continue;
		}

		switch (cmd) {
		case 1:
			freePeopleList(people, n);
			n = loadPeopleFromDatabase(people);
			initMatchingInfos(infos, people, n);
			result_cnt = 0;
			current_idx = -1;
			break;
		case 2:
			printUsers(infos, n);
			break;
		case 3:
			current_idx = chooseCurrentUser(infos, n);
			break;
		case 4:
			result_cnt = runStableMatching(infos, n, result);
			break;
		case 5:
			respondToMyProposal(infos, n, result, result_cnt, current_idx);
			break;
		case 6:
			freePeopleList(people, n);
			free(infos);
			free(result);
			printf("Exit.\n");
			return 0;
		default:
			printf("Invalid command.\n");
			break;
		}
	}
}
