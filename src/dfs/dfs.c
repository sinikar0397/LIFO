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

const char *dfs_type_name(const char type[]) {
	return getNode(dfs_find_type_node(type))->name;
}

int compat(People *a, People *b) {
	if (a == NULL || b == NULL) {
		return 0;
	}

	return dfs_type_similarity(a->love_type, b->type) +
		   dfs_type_similarity(b->love_type, a->type);
}

// ───────────────────────────────────────────────
// 트리 엔진
// ───────────────────────────────────────────────

// 내부(분기) 노드를 채우는 헬퍼: 2지선다용.
static void dfs_set_branch(DfsTreeNode *n, const char *q, const char *o0,
						   int c0, const char *o1, int c1) {
	n->is_leaf = 0;
	n->user_added = 0;
	n->n_opt = 2;
	strncpy(n->question, q, DFS_Q_LEN - 1);
	strncpy(n->opt_text[0], o0, DFS_OPT_LEN - 1);
	n->child[0] = c0;
	strncpy(n->opt_text[1], o1, DFS_OPT_LEN - 1);
	n->child[1] = c1;
}

// 잎 노드를 채우는 헬퍼.
static void dfs_set_leaf(DfsTreeNode *n, const char *code, const char *name) {
	n->is_leaf = 1;
	n->user_added = 0;
	n->n_opt = 0;
	strncpy(n->code, code, MAX_TYPE_LEN - 1);
	strncpy(n->name, name, DFS_NAME_LEN - 1);
}

// 잎 8종(코드/이름)은 두 트리가 공유한다.
static void dfs_fill_leaves(DfsTree *t) {
	dfs_set_leaf(&t->nodes[7], "DTF", "이성 리드 정면 돌파형");
	dfs_set_leaf(&t->nodes[8], "DTC", "이성 리드 회피 조정형");
	dfs_set_leaf(&t->nodes[9], "DEF", "감성 케어 정면 돌파형");
	dfs_set_leaf(&t->nodes[10], "DEC", "감성 케어 회피 안정형");
	dfs_set_leaf(&t->nodes[11], "STF", "이성 수용 정면 돌파형");
	dfs_set_leaf(&t->nodes[12], "STC", "이성 수용 회피 관찰형");
	dfs_set_leaf(&t->nodes[13], "SEF", "감성 의존 정면 돌파형");
	dfs_set_leaf(&t->nodes[14], "SEC", "감성 의존 회피 보호형");
}

void dfs_build_self_tree(DfsTree *t) {
	memset(t, 0, sizeof(*t));
	strncpy(t->title, "주도·표현 성향", sizeof(t->title) - 1);
	t->root = 0;
	t->n_nodes = 15;

	dfs_set_branch(&t->nodes[0], "관계에서 더 자연스러운 내 모습은?",
				   "내가 먼저 데이트·대화·문제 해결을 제안한다", 1,
				   "상대의 제안과 분위기를 존중하며 맞춘다", 2);
	dfs_set_branch(&t->nodes[1], "애정을 표현하거나 갈등을 다룰 때 더 가까운 방식은?",
				   "사실·기준·계획을 정리해서 관계를 이끈다", 3,
				   "감정 확인·위로·케어로 관계를 이끈다", 4);
	dfs_set_branch(&t->nodes[2], "상대를 따라갈 때 더 가까운 방식은?",
				   "논리적으로 납득되면 수용한다", 5,
				   "정서적으로 기대고 친밀감을 확인한다", 6);
	const char *aq = "문제가 생겼을 때 내 행동은?";
	const char *af = "바로 마주 보고 이야기해서 푼다 (정면 돌파)";
	const char *ac = "잠시 거리를 두고 감정을 가라앉힌 뒤 다룬다 (회피·조정)";
	dfs_set_branch(&t->nodes[3], aq, af, 7, ac, 8);
	dfs_set_branch(&t->nodes[4], aq, af, 9, ac, 10);
	dfs_set_branch(&t->nodes[5], aq, af, 11, ac, 12);
	dfs_set_branch(&t->nodes[6], aq, af, 13, ac, 14);

	dfs_fill_leaves(t);
}

void dfs_build_ideal_tree(DfsTree *t) {
	memset(t, 0, sizeof(*t));
	strncpy(t->title, "원하는 주도성", sizeof(t->title) - 1);
	t->root = 0;
	t->n_nodes = 15;

	dfs_set_branch(&t->nodes[0], "내가 더 끌리는 상대의 모습은?",
				   "관계를 주도하며 방향을 제안하는 사람", 1,
				   "내 흐름을 존중하고 맞춰주는 사람", 2);
	dfs_set_branch(&t->nodes[1], "그 사람이 관계를 이끄는 방식은 어느 쪽이 좋아?",
				   "논리와 계획으로 이끄는 사람", 3,
				   "감정 확인과 케어로 이끄는 사람", 4);
	dfs_set_branch(&t->nodes[2], "맞춰주는 방식은 어느 쪽이 좋아?",
				   "차분히 납득하고 수용하는 사람", 5,
				   "정서적으로 기대오며 친밀감을 표현하는 사람", 6);
	const char *aq = "갈등 상황에서 상대가 이랬으면 좋겠어:";
	const char *af = "바로 마주 보고 풀어가는 사람 (정면 돌파)";
	const char *ac = "잠시 거리를 두고 추스른 뒤 다루는 사람 (회피·조정)";
	dfs_set_branch(&t->nodes[3], aq, af, 7, ac, 8);
	dfs_set_branch(&t->nodes[4], aq, af, 9, ac, 10);
	dfs_set_branch(&t->nodes[5], aq, af, 11, ac, 12);
	dfs_set_branch(&t->nodes[6], aq, af, 13, ac, 14);

	dfs_fill_leaves(t);
}

static int dfs_node_depth(const DfsTree *t, int idx) {
	const DfsTreeNode *n = &t->nodes[idx];
	if (n->is_leaf) {
		return 0;
	}
	int best = 0;
	for (int i = 0; i < n->n_opt; i++) {
		int d = dfs_node_depth(t, n->child[i]);
		if (d > best) {
			best = d;
		}
	}
	return best + 1;
}

int dfs_tree_max_depth(const DfsTree *t) {
	return dfs_node_depth(t, t->root);
}

// 애착 안정성 잎 4종(코드/이름)은 두 트리가 공유.
static void dfs_fill_attach_leaves(DfsTree *t) {
	dfs_set_leaf(&t->nodes[3], "AS", "안정 애착형");
	dfs_set_leaf(&t->nodes[4], "AV", "회피 애착형");
	dfs_set_leaf(&t->nodes[5], "AX", "불안 애착형");
	dfs_set_leaf(&t->nodes[6], "FA", "혼란 애착형");
}

void dfs_build_attach_self_tree(DfsTree *t) {
	memset(t, 0, sizeof(*t));
	strncpy(t->title, "애착 안정성", sizeof(t->title) - 1);
	t->root = 0;
	t->n_nodes = 7;
	dfs_set_branch(&t->nodes[0], "연인과 잠시 떨어져 있을 때 내 마음은?",
				   "별 걱정 없이 내 일에 집중하는 편", 1,
				   "연락이나 반응이 자꾸 신경 쓰이는 편", 2);
	dfs_set_branch(&t->nodes[1], "관계가 더 가까워지는 것에 대해선?",
				   "편하게 받아들이고 잘 맞춘다", 3,
				   "너무 가까워지면 부담돼서 거리를 둔다", 4);
	dfs_set_branch(&t->nodes[2], "관계가 더 가까워지는 것에 대해선?",
				   "더 가까워지고 자주 확인받고 싶다", 5,
				   "원하면서도 상처받을까 봐 망설인다", 6);
	dfs_fill_attach_leaves(t);
}

void dfs_build_attach_ideal_tree(DfsTree *t) {
	memset(t, 0, sizeof(*t));
	strncpy(t->title, "애착 안정성", sizeof(t->title) - 1);
	t->root = 0;
	t->n_nodes = 7;
	dfs_set_branch(&t->nodes[0], "내가 더 편한 상대는?",
				   "각자의 시간을 존중해 주는 사람", 1,
				   "자주 표현하고 확인해 주는 사람", 2);
	dfs_set_branch(&t->nodes[1], "가까워지는 속도는 어느 쪽이 좋아?",
				   "서두르지 않고 편하게 맞춰가는 사람", 3,
				   "적당히 거리를 두는 게 편한 사람", 4);
	dfs_set_branch(&t->nodes[2], "애정 표현은 어느 쪽이 좋아?",
				   "적극적으로 다가오고 확인시켜 주는 사람", 5,
				   "마음은 깊지만 조심스럽게 표현하는 사람", 6);
	dfs_fill_attach_leaves(t);
}

// 기본 트리를 빌드한 뒤, 저장 파일이 있으면 덮어쓰고, 마지막에 경로를 박아둔다.
static void dfs_load_or_keep(DfsTree *t, const char *path) {
	dfs_load_tree(t, path); // 파일 있으면 t를 덮어씀, 없으면 기본 트리 유지
	strncpy(t->save_path, path, sizeof(t->save_path) - 1);
	t->save_path[sizeof(t->save_path) - 1] = '\0';
}

void dfs_build_self_survey(DfsSurvey *s) {
	memset(s, 0, sizeof(*s));
	strncpy(s->name, "내 연애 성향", sizeof(s->name) - 1);
	s->n_trees = 2;
	dfs_build_self_tree(&s->trees[0]);
	dfs_load_or_keep(&s->trees[0], "database/tree_self_0.json");
	dfs_build_attach_self_tree(&s->trees[1]);
	dfs_load_or_keep(&s->trees[1], "database/tree_self_1.json");
}

void dfs_build_ideal_survey(DfsSurvey *s) {
	memset(s, 0, sizeof(*s));
	strncpy(s->name, "내 이상형", sizeof(s->name) - 1);
	s->n_trees = 2;
	dfs_build_ideal_tree(&s->trees[0]);
	dfs_load_or_keep(&s->trees[0], "database/tree_ideal_0.json");
	dfs_build_attach_ideal_tree(&s->trees[1]);
	dfs_load_or_keep(&s->trees[1], "database/tree_ideal_1.json");
}

int dfs_find_leaf_by_code(const DfsTree *t, const char code[]) {
	for (int i = 0; i < t->n_nodes; i++) {
		if (t->nodes[i].is_leaf && strcmp(t->nodes[i].code, code) == 0) {
			return i;
		}
	}
	return -1;
}

int dfs_extend_leaf(DfsTree *t, int leaf_idx, const char question[],
					const char opt0[], const char opt1[]) {
	if (leaf_idx < 0 || leaf_idx >= t->n_nodes) {
		return 0;
	}
	if (!t->nodes[leaf_idx].is_leaf) {
		return 0;
	}
	if (t->n_nodes + 2 > DFS_TREE_MAX_NODES) {
		return 0;
	}

	char pcode[MAX_TYPE_LEN], pname[DFS_NAME_LEN];
	strncpy(pcode, t->nodes[leaf_idx].code, MAX_TYPE_LEN - 1);
	pcode[MAX_TYPE_LEN - 1] = '\0';
	strncpy(pname, t->nodes[leaf_idx].name, DFS_NAME_LEN - 1);
	pname[DFS_NAME_LEN - 1] = '\0';

	int c0 = t->n_nodes, c1 = t->n_nodes + 1;
	char code[MAX_TYPE_LEN], name[DFS_NAME_LEN];

	snprintf(code, sizeof(code), "%s1", pcode);
	snprintf(name, sizeof(name), "%s · %s", pname, opt0);
	dfs_set_leaf(&t->nodes[c0], code, name);
	t->nodes[c0].user_added = 1;

	snprintf(code, sizeof(code), "%s2", pcode);
	snprintf(name, sizeof(name), "%s · %s", pname, opt1);
	dfs_set_leaf(&t->nodes[c1], code, name);
	t->nodes[c1].user_added = 1;

	// 기존 잎을 분기 노드로 변환
	dfs_set_branch(&t->nodes[leaf_idx], question, opt0, c0, opt1, c1);
	t->nodes[leaf_idx].user_added = 1;

	t->n_nodes += 2;
	return 1;
}

int dfs_save_tree(const DfsTree *t, const char path[]) {
	cJSON *root = cJSON_CreateObject();
	if (root == NULL) {
		return 0;
	}
	cJSON_AddStringToObject(root, "title", t->title);
	cJSON_AddNumberToObject(root, "root", t->root);
	cJSON_AddNumberToObject(root, "n_nodes", t->n_nodes);

	cJSON *arr = cJSON_CreateArray();
	cJSON_AddItemToObject(root, "nodes", arr);
	for (int i = 0; i < t->n_nodes; i++) {
		const DfsTreeNode *n = &t->nodes[i];
		cJSON *o = cJSON_CreateObject();
		cJSON_AddNumberToObject(o, "is_leaf", n->is_leaf);
		cJSON_AddStringToObject(o, "question", n->question);
		cJSON_AddNumberToObject(o, "n_opt", n->n_opt);
		cJSON *ot = cJSON_CreateArray();
		cJSON *ch = cJSON_CreateArray();
		cJSON_AddItemToObject(o, "opt_text", ot);
		cJSON_AddItemToObject(o, "child", ch);
		for (int k = 0; k < n->n_opt; k++) {
			cJSON_AddItemToArray(ot, cJSON_CreateString(n->opt_text[k]));
			cJSON_AddItemToArray(ch, cJSON_CreateNumber(n->child[k]));
		}
		cJSON_AddStringToObject(o, "code", n->code);
		cJSON_AddStringToObject(o, "name", n->name);
		cJSON_AddNumberToObject(o, "user_added", n->user_added);
		cJSON_AddItemToArray(arr, o);
	}

	char *txt = cJSON_Print(root);
	cJSON_Delete(root);
	if (txt == NULL) {
		return 0;
	}
	FILE *f = fopen(path, "w");
	if (f == NULL) {
		free(txt);
		return 0;
	}
	fputs(txt, f);
	fclose(f);
	free(txt);
	return 1;
}

int dfs_load_tree(DfsTree *t, const char path[]) {
	FILE *f = fopen(path, "r");
	if (f == NULL) {
		return 0; // 파일 없음 → 기본 트리 유지
	}
	fseek(f, 0, SEEK_END);
	long sz = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (sz <= 0) {
		fclose(f);
		return 0;
	}
	char *buf = malloc(sz + 1);
	if (buf == NULL) {
		fclose(f);
		return 0;
	}
	size_t rd = fread(buf, 1, sz, f);
	buf[rd] = '\0';
	fclose(f);

	cJSON *root = cJSON_Parse(buf);
	free(buf);
	if (root == NULL) {
		return 0;
	}

	memset(t, 0, sizeof(*t));
	cJSON *title = cJSON_GetObjectItem(root, "title");
	if (cJSON_IsString(title)) {
		strncpy(t->title, title->valuestring, sizeof(t->title) - 1);
	}
	cJSON *rootn = cJSON_GetObjectItem(root, "root");
	t->root = cJSON_IsNumber(rootn) ? rootn->valueint : 0;

	cJSON *arr = cJSON_GetObjectItem(root, "nodes");
	int i = 0;
	if (cJSON_IsArray(arr)) {
		cJSON *o = NULL;
		cJSON_ArrayForEach(o, arr) {
			if (i >= DFS_TREE_MAX_NODES) {
				break;
			}
			DfsTreeNode *n = &t->nodes[i];
			cJSON *il = cJSON_GetObjectItem(o, "is_leaf");
			n->is_leaf = cJSON_IsNumber(il) ? il->valueint : 0;
			cJSON *q = cJSON_GetObjectItem(o, "question");
			if (cJSON_IsString(q)) {
				strncpy(n->question, q->valuestring, DFS_Q_LEN - 1);
			}
			cJSON *no = cJSON_GetObjectItem(o, "n_opt");
			n->n_opt = cJSON_IsNumber(no) ? no->valueint : 0;
			if (n->n_opt > DFS_MAX_OPT) {
				n->n_opt = DFS_MAX_OPT;
			}
			cJSON *ot = cJSON_GetObjectItem(o, "opt_text");
			cJSON *ch = cJSON_GetObjectItem(o, "child");
			for (int k = 0; k < n->n_opt; k++) {
				cJSON *otk = cJSON_GetArrayItem(ot, k);
				if (cJSON_IsString(otk)) {
					strncpy(n->opt_text[k], otk->valuestring, DFS_OPT_LEN - 1);
				}
				cJSON *chk = cJSON_GetArrayItem(ch, k);
				n->child[k] = cJSON_IsNumber(chk) ? chk->valueint : 0;
			}
			cJSON *code = cJSON_GetObjectItem(o, "code");
			if (cJSON_IsString(code)) {
				strncpy(n->code, code->valuestring, MAX_TYPE_LEN - 1);
			}
			cJSON *name = cJSON_GetObjectItem(o, "name");
			if (cJSON_IsString(name)) {
				strncpy(n->name, name->valuestring, DFS_NAME_LEN - 1);
			}
			cJSON *ua = cJSON_GetObjectItem(o, "user_added");
			n->user_added = cJSON_IsNumber(ua) ? ua->valueint : 0;
			i++;
		}
	}
	t->n_nodes = i;
	cJSON_Delete(root);
	return 1;
}
