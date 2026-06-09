#include "dfs.h"

typedef struct DateFitNode {
	DateFitNodeId id;
	DateFitNodeId parent;
	const char *code;
	const char *name;
	const char *summary;
} DateFitNode;

static const DateFitNode DATE_FIT_TREE[DFS_NODE_COUNT] = {
	{DFS_NODE_ROOT, DFS_NODE_ROOT, "ROOT", "연애 성향", "전체 성향의 루트"},
	{DFS_NODE_DOMINANT, DFS_NODE_ROOT, "D", "지배/리드형", "관계를 주도하고 방향을 제안하는 편"},
	{DFS_NODE_SUBMISSIVE, DFS_NODE_ROOT, "S", "피지배/수용형", "상대의 흐름을 존중하고 맞춰가는 편"},
	{DFS_NODE_DOMINANT_THINKING, DFS_NODE_DOMINANT, "DT", "이성적 리드", "논리와 계획으로 관계를 이끄는 편"},
	{DFS_NODE_DOMINANT_EMOTIONAL, DFS_NODE_DOMINANT, "DE", "감성적 케어", "정서적 반응과 돌봄으로 관계를 이끄는 편"},
	{DFS_NODE_SUBMISSIVE_THINKING, DFS_NODE_SUBMISSIVE, "ST", "이성적 수용", "차분히 판단하며 상대의 제안을 받아들이는 편"},
	{DFS_NODE_SUBMISSIVE_EMOTIONAL, DFS_NODE_SUBMISSIVE, "SE", "감성적 의존", "감정적 교감과 안정감을 중요하게 여기는 편"},
	{DFS_NODE_DTF, DFS_NODE_DOMINANT_THINKING, "DTF", "이성 리드 정면 돌파형", "문제를 빠르게 마주하고 해결책을 제시한다"},
	{DFS_NODE_DTC, DFS_NODE_DOMINANT_THINKING, "DTC", "이성 리드 회피 조정형", "갈등을 식힌 뒤 논리적으로 조율한다"},
	{DFS_NODE_DEF, DFS_NODE_DOMINANT_EMOTIONAL, "DEF", "감성 케어 정면 돌파형", "감정을 바로 확인하고 따뜻하게 풀어낸다"},
	{DFS_NODE_DEC, DFS_NODE_DOMINANT_EMOTIONAL, "DEC", "감성 케어 회피 안정형", "상대가 편해질 때까지 기다리며 보듬는다"},
	{DFS_NODE_STF, DFS_NODE_SUBMISSIVE_THINKING, "STF", "이성 수용 정면 돌파형", "상대 의견을 듣되 필요한 말은 분명히 한다"},
	{DFS_NODE_STC, DFS_NODE_SUBMISSIVE_THINKING, "STC", "이성 수용 회피 관찰형", "거리를 두고 상황을 분석한 뒤 반응한다"},
	{DFS_NODE_SEF, DFS_NODE_SUBMISSIVE_EMOTIONAL, "SEF", "감성 의존 정면 돌파형", "불안을 숨기지 않고 애정을 확인한다"},
	{DFS_NODE_SEC, DFS_NODE_SUBMISSIVE_EMOTIONAL, "SEC", "감성 의존 회피 보호형", "상처를 피하며 안정적인 애착을 원한다"},
};

static void clearInputBuffer(void) {
	int ch;
	while ((ch = getchar()) != '\n' && ch != EOF) {
	}
}

static int readIntInRange(int min_value, int max_value) {
	int value;

	while (1) {
		printf("입력 : ");
		if (scanf("%d", &value) == 1 && min_value <= value && value <= max_value) {
			return value;
		}
		clearInputBuffer();
		printf("%d부터 %d 사이의 숫자를 입력해주세요.\n", min_value, max_value);
	}
}

static const DateFitNode *getNode(DateFitNodeId id) {
	if (id < 0 || id >= DFS_NODE_COUNT) {
		return &DATE_FIT_TREE[DFS_NODE_ROOT];
	}
	return &DATE_FIT_TREE[id];
}

static DateFitNodeId chooseMajorBranch(void) {
	printf("\n[대분류] 관계에서 더 자연스러운 모습은 무엇인가요?\n");
	printf("1. 내가 먼저 데이트 방향, 대화 주제, 문제 해결을 제안한다.\n");
	printf("2. 상대의 제안과 분위기를 존중하며 흐름을 맞춘다.\n");
	return readIntInRange(1, 2) == 1 ? DFS_NODE_DOMINANT : DFS_NODE_SUBMISSIVE;
}

static DateFitNodeId chooseMiddleBranch(DateFitNodeId major) {
	printf("\n[중분류] 애정을 표현하거나 갈등을 다룰 때 더 가까운 방식은?\n");
	if (major == DFS_NODE_DOMINANT) {
		printf("1. 사실, 기준, 계획을 정리해서 관계를 이끈다.\n");
		printf("2. 감정 확인, 위로, 케어로 관계를 이끈다.\n");
		return readIntInRange(1, 2) == 1 ? DFS_NODE_DOMINANT_THINKING
										 : DFS_NODE_DOMINANT_EMOTIONAL;
	}

	printf("1. 상대 의견을 듣고 논리적으로 납득되면 수용한다.\n");
	printf("2. 상대에게 정서적으로 기대고 친밀감을 확인한다.\n");
	return readIntInRange(1, 2) == 1 ? DFS_NODE_SUBMISSIVE_THINKING
									 : DFS_NODE_SUBMISSIVE_EMOTIONAL;
}

static DateFitNodeId chooseActionBranch(DateFitNodeId middle) {
	int face_to_face;

	printf("\n[소분류] 문제가 생겼을 때 행동 방식은?\n");
	printf("1. 바로 마주 보고 이야기해서 풀고 싶다. (F: 정면 돌파)\n");
	printf("2. 잠시 거리를 두고 감정을 가라앉힌 뒤 다룬다. (C: 회피/조정)\n");
	face_to_face = readIntInRange(1, 2) == 1;

	switch (middle) {
	case DFS_NODE_DOMINANT_THINKING:
		return face_to_face ? DFS_NODE_DTF : DFS_NODE_DTC;
	case DFS_NODE_DOMINANT_EMOTIONAL:
		return face_to_face ? DFS_NODE_DEF : DFS_NODE_DEC;
	case DFS_NODE_SUBMISSIVE_THINKING:
		return face_to_face ? DFS_NODE_STF : DFS_NODE_STC;
	case DFS_NODE_SUBMISSIVE_EMOTIONAL:
		return face_to_face ? DFS_NODE_SEF : DFS_NODE_SEC;
	default:
		return DFS_NODE_ROOT;
	}
}

void dfs_run_date_fit_survey(char result_type[MAX_TYPE_LEN], const char title[]) {
	DateFitNodeId major;
	DateFitNodeId middle;
	DateFitNodeId leaf;

	if (title != NULL && title[0] != '\0') {
		printf("\n========================================\n");
		printf(" %s\n", title);
		printf("========================================\n");
	}

	major = chooseMajorBranch();
	middle = chooseMiddleBranch(major);
	leaf = chooseActionBranch(middle);

	strncpy(result_type, getNode(leaf)->code, MAX_TYPE_LEN - 1);
	result_type[MAX_TYPE_LEN - 1] = '\0';

	printf("\n산출된 성격 유형: %s\n", result_type);
	dfs_print_type_description(result_type);
	dfs_customize_date_fit_type(result_type);
}

void dfs_customize_date_fit_type(char result_type[MAX_TYPE_LEN]) {
	DateFitNodeId current;
	int cmd;

	printf("\n추가 질문으로 결과를 커스터마이징할까요?\n");
	printf("1. 아니오, 현재 결과를 사용합니다.\n");
	printf("2. 예, 일부 분기점을 다시 고릅니다.\n");
	if (readIntInRange(1, 2) == 1) {
		return;
	}

	current = dfs_find_type_node(result_type);
	if (current == DFS_NODE_ROOT) {
		current = DFS_NODE_DTF;
	}

	while (1) {
		DateFitNodeId major = getNode(getNode(current)->parent)->parent;
		DateFitNodeId middle = getNode(current)->parent;

		printf("\n현재 유형: %s\n", getNode(current)->code);
		dfs_print_type_description(getNode(current)->code);
		printf("1. 대분류 다시 선택\n");
		printf("2. 중분류 다시 선택\n");
		printf("3. 소분류 다시 선택\n");
		printf("4. 커스터마이징 완료\n");
		cmd = readIntInRange(1, 4);

		if (cmd == 4) {
			break;
		}
		if (cmd == 1) {
			major = chooseMajorBranch();
			middle = chooseMiddleBranch(major);
			current = chooseActionBranch(middle);
		} else if (cmd == 2) {
			middle = chooseMiddleBranch(major);
			current = chooseActionBranch(middle);
		} else {
			current = chooseActionBranch(middle);
		}
	}

	strncpy(result_type, getNode(current)->code, MAX_TYPE_LEN - 1);
	result_type[MAX_TYPE_LEN - 1] = '\0';
	printf("\n최종 성격 유형: %s\n", result_type);
	dfs_print_type_description(result_type);
}

DateFitNodeId dfs_find_type_node(const char type[]) {
	if (type == NULL) {
		return DFS_NODE_ROOT;
	}

	for (int i = 0; i < DFS_NODE_COUNT; i++) {
		if (strcmp(type, DATE_FIT_TREE[i].code) == 0) {
			return DATE_FIT_TREE[i].id;
		}
	}

	return DFS_NODE_ROOT;
}

int dfs_depth(DateFitNodeId node) {
	int depth = 0;

	if (node < 0 || node >= DFS_NODE_COUNT) {
		return 0;
	}

	while (node != DFS_NODE_ROOT) {
		node = getNode(node)->parent;
		depth++;
	}

	return depth;
}

DateFitNodeId dfs_lca(DateFitNodeId u, DateFitNodeId v) {
	int depth_u = dfs_depth(u);
	int depth_v = dfs_depth(v);

	while (depth_u > depth_v) {
		u = getNode(u)->parent;
		depth_u--;
	}
	while (depth_v > depth_u) {
		v = getNode(v)->parent;
		depth_v--;
	}
	while (u != v) {
		u = getNode(u)->parent;
		v = getNode(v)->parent;
	}

	return u;
}

int dfs_distance(DateFitNodeId u, DateFitNodeId v) {
	DateFitNodeId lca = dfs_lca(u, v);
	return dfs_depth(u) + dfs_depth(v) - 2 * dfs_depth(lca);
}

int dfs_type_distance(const char type_a[], const char type_b[]) {
	return dfs_distance(dfs_find_type_node(type_a), dfs_find_type_node(type_b));
}

int dfs_type_similarity(const char ideal_type[], const char other_type[]) {
	int distance = dfs_type_distance(ideal_type, other_type);
	int max_distance = 2 * dfs_depth(DFS_NODE_DTF);

	if (max_distance == 0) {
		return 0;
	}
	if (distance > max_distance) {
		distance = max_distance;
	}

	return (max_distance - distance) * 100 / max_distance;
}

void dfs_print_type_description(const char type[]) {
	DateFitNodeId node = dfs_find_type_node(type);
	const DateFitNode *info = getNode(node);

	printf("- 이름: %s\n", info->name);
	printf("- 설명: %s\n", info->summary);
}

int compat(People *a, People *b) {
	if (a == NULL || b == NULL) {
		return 0;
	}

	return dfs_type_similarity(a->love_type, b->type) +
		   dfs_type_similarity(b->love_type, a->type);
}
