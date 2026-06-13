#include "dfs.h"
#include "dfs_func.h"

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
	{DFS_NODE_DTF, DFS_NODE_DOMINANT_THINKING, "DTF", "번개 지휘자", "문제를 빠르게 마주하고 해결책을 제시한다"},
	{DFS_NODE_DTC, DFS_NODE_DOMINANT_THINKING, "DTC", "체스 전략가", "갈등을 식힌 뒤 논리적으로 조율한다"},
	{DFS_NODE_DEF, DFS_NODE_DOMINANT_EMOTIONAL, "DEF", "따뜻한 횃불", "감정을 바로 확인하고 따뜻하게 풀어낸다"},
	{DFS_NODE_DEC, DFS_NODE_DOMINANT_EMOTIONAL, "DEC", "포근한 담요", "상대가 편해질 때까지 기다리며 보듬는다"},
	{DFS_NODE_STF, DFS_NODE_SUBMISSIVE_THINKING, "STF", "맑은 거울", "상대 의견을 듣되 필요한 말은 분명히 한다"},
	{DFS_NODE_STC, DFS_NODE_SUBMISSIVE_THINKING, "STC", "조용한 관찰자", "거리를 두고 상황을 분석한 뒤 반응한다"},
	{DFS_NODE_SEF, DFS_NODE_SUBMISSIVE_EMOTIONAL, "SEF", "솔직한 햇살", "불안을 숨기지 않고 애정을 확인한다"},
	{DFS_NODE_SEC, DFS_NODE_SUBMISSIVE_EMOTIONAL, "SEC", "달빛 그림자", "상처를 피하며 안정적인 애착을 원한다"},
};

static const DateFitNode *getNode(DateFitNodeId id) {
	if (id < 0 || id >= DFS_NODE_COUNT) {
		return &DATE_FIT_TREE[DFS_NODE_ROOT];
	}
	return &DATE_FIT_TREE[id];
}

// ── 매칭/표시용 공용 트리 캐시 ──
// compat은 BFS에서 O(n^2)로, dfs_type_name은 화면마다 불리므로, 세분화가 누적된
// 공용 설문 트리를 매번 로드하지 않고 한 번만 빌드·로드해 캐시한다.
// trees[0]=성향, trees[1]=애착, trees[2]=사랑의 언어.
static DfsSurvey g_match_survey;
static int g_match_ready = 0;

static void dfs_matching_ensure(void) {
	if (!g_match_ready) {
		dfs_build_self_survey(&g_match_survey); // 기본 트리 + JSON 세분화 로드
		g_match_ready = 1;
	}
}

// 세분화 등으로 트리 파일이 바뀌면 호출 → 다음 조회 때 다시 로드.
void dfs_matching_reload(void) { g_match_ready = 0; }

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
	if (type == NULL || type[0] == '\0') {
		return "미설정";
	}
	// 공용 설문 트리(성향·애착·사랑의 언어)에서 코드에 맞는 잎 이름을 찾는다.
	// 세분화로 추가된 유형명도 여기서 해결된다.
	dfs_matching_ensure();
	for (int t = 0; t < g_match_survey.n_trees; t++) {
		int li = dfs_find_leaf_by_code(&g_match_survey.trees[t], type);
		if (li >= 0) {
			return g_match_survey.trees[t].nodes[li].name;
		}
	}
	// 트리에서 못 찾으면 레거시 하드코딩 테이블로 폴백.
	return getNode(dfs_find_type_node(type))->name;
}

const char *dfs_type_desc(const char code[]) {
	if (code == NULL || code[0] == '\0') {
		return "";
	}
	dfs_matching_ensure();
	for (int t = 0; t < g_match_survey.n_trees; t++) {
		int li = dfs_find_leaf_by_code(&g_match_survey.trees[t], code);
		if (li >= 0) {
			return g_match_survey.trees[t].nodes[li].desc;
		}
	}
	return getNode(dfs_find_type_node(code))->summary;
}

// ───────────────────────────────────────────────
// 애착 안정성 차원 (AS/AV/AX/FA)
// 주도·표현 성향과는 별개의 트리. 매칭 호감도에 함께 합산한다.
//   ROOT ─ LOW(낮은 불안) ─ {AS 안정, AV 회피}
//        └ HIGH(높은 불안) ─ {AX 불안, FA 혼란}
// ───────────────────────────────────────────────
typedef enum AttachNodeId {
	ATT_ROOT, ATT_LOW, ATT_HIGH, ATT_AS, ATT_AV, ATT_AX, ATT_FA, ATT_COUNT
} AttachNodeId;

static const struct {
	int parent;
	const char *code;
	const char *name;
} ATTACH_TREE[ATT_COUNT] = {
	{ATT_ROOT, "ROOT", "애착 안정성"},
	{ATT_ROOT, "LOW", "낮은 불안"},
	{ATT_ROOT, "HIGH", "높은 불안"},
	{ATT_LOW, "AS", "안정 애착형"},
	{ATT_LOW, "AV", "회피 애착형"},
	{ATT_HIGH, "AX", "불안 애착형"},
	{ATT_HIGH, "FA", "혼란 애착형"},
};

static AttachNodeId attach_find(const char code[]) {
	if (code != NULL) {
		for (int i = 0; i < ATT_COUNT; i++) {
			if (strcmp(code, ATTACH_TREE[i].code) == 0) {
				return (AttachNodeId)i;
			}
		}
	}
	return ATT_ROOT;
}

static int attach_depth(AttachNodeId n) {
	int d = 0;
	while (n != ATT_ROOT) {
		n = ATTACH_TREE[n].parent;
		d++;
	}
	return d;
}

static AttachNodeId attach_lca(AttachNodeId u, AttachNodeId v) {
	int du = attach_depth(u), dv = attach_depth(v);
	while (du > dv) {
		u = ATTACH_TREE[u].parent;
		du--;
	}
	while (dv > du) {
		v = ATTACH_TREE[v].parent;
		dv--;
	}
	while (u != v) {
		u = ATTACH_TREE[u].parent;
		v = ATTACH_TREE[v].parent;
	}
	return u;
}

int dfs_attach_similarity(const char ideal[], const char other[]) {
	AttachNodeId a = attach_find(ideal), b = attach_find(other);
	AttachNodeId l = attach_lca(a, b);
	int dist = attach_depth(a) + attach_depth(b) - 2 * attach_depth(l);
	int max_dist = 2 * attach_depth(ATT_AS); // 서로 다른 가지의 잎 = 4

	if (max_dist == 0) {
		return 0;
	}
	if (dist > max_dist) {
		dist = max_dist;
	}
	return (max_dist - dist) * 100 / max_dist;
}

const char *dfs_attach_name(const char code[]) {
	return ATTACH_TREE[attach_find(code)].name;
}

// 한쪽 방향(ideal 쪽이 상대 other를 얼마나 원하는지)의 유사도(0~100).
// 차원(성향·애착·사랑의 언어)별 generic 트리 유사도를 평균한다. 트리에서 직접
// 계산하므로 세분화 유형도 그대로 반영. 양쪽 다 값이 있는 차원만 합산한다.
static int compat_dir(const char *ideal[], const char *other[]) {
	dfs_matching_ensure();
	int sum = 0, cnt = 0;
	int dims = g_match_survey.n_trees < DFS_MAX_TREES ? g_match_survey.n_trees
													  : DFS_MAX_TREES;
	for (int d = 0; d < dims; d++) {
		if (ideal[d] && ideal[d][0] && other[d] && other[d][0]) {
			sum += dfs_tree_similarity(&g_match_survey.trees[d], ideal[d],
									   other[d]);
			cnt++;
		}
	}
	return cnt ? sum / cnt : 0;
}

int compat(People *a, People *b) {
	if (a == NULL || b == NULL) {
		return 0;
	}
	// 차원 순서: 성향, 애착, 사랑의 언어 (트리 인덱스와 일치)
	const char *a_ideal[DFS_MAX_TREES] = {a->love_type, a->love_attach,
										  a->love_lang};
	const char *a_self[DFS_MAX_TREES] = {a->type, a->attach, a->lang};
	const char *b_ideal[DFS_MAX_TREES] = {b->love_type, b->love_attach,
										  b->love_lang};
	const char *b_self[DFS_MAX_TREES] = {b->type, b->attach, b->lang};

	return compat_dir(a_ideal, b_self) + compat_dir(b_ideal, a_self);
}

// ───────────────────────────────────────────────
// 트리 엔진
// ───────────────────────────────────────────────

// 잎 8종(코드/이름)은 두 트리가 공유한다.
// 잎 설정 + 설명까지 한 번에.
static void dfs_leaf(DfsTree *t, int idx, const char *code, const char *name,
					 const char *desc) {
	dfs_set_leaf(&t->nodes[idx], code, name);
	strncpy(t->nodes[idx].desc, desc, DFS_DESC_LEN - 1);
	t->nodes[idx].desc[DFS_DESC_LEN - 1] = '\0';
}

static void dfs_fill_leaves(DfsTree *t) {
	dfs_leaf(t, 7, "DTF", "번개 지휘자",
			 "관계를 주도하고 논리로 판단하며, 문제가 생기면 곧장 부딪쳐 "
			 "해결한다. 빠르고 명쾌하지만 가끔 상대의 속도를 앞지른다.");
	dfs_leaf(t, 8, "DTC", "체스 전략가",
			 "주도하되 감정을 식힌 뒤 논리로 조율한다. 충동보다 계획, "
			 "즉답보다 한 수 앞을 내다보는 신중한 리더형.");
	dfs_leaf(t, 9, "DEF", "따뜻한 횃불",
			 "감정을 먼저 읽고 다가가, 문제를 따뜻하게 정면으로 풀어낸다. "
			 "표현이 솔직하고 적극적이라 곁에 있으면 든든하다.");
	dfs_leaf(t, 10, "DEC", "포근한 담요",
			 "상대를 보듬으며 이끌되, 갈등은 서두르지 않고 편안해질 때까지 "
			 "기다린다. 잔잔한 안정감을 주는 케어형.");
	dfs_leaf(t, 11, "STF", "맑은 거울",
			 "상대의 흐름을 존중해 맞추지만 필요한 말은 분명히 한다. "
			 "차분하면서도 솔직해 관계가 투명하다.");
	dfs_leaf(t, 12, "STC", "조용한 관찰자",
			 "한 발 물러나 상황을 분석한 뒤 반응한다. 신중하고 침착하지만 "
			 "속마음 표현은 아껴 천천히 가까워진다.");
	dfs_leaf(t, 13, "SEF", "솔직한 햇살",
			 "감정에 솔직하고 애정을 자주 확인한다. 맞춰주면서도 마음을 "
			 "숨기지 않아 따뜻함이 그대로 전해진다.");
	dfs_leaf(t, 14, "SEC", "달빛 그림자",
			 "깊이 정들지만 상처를 피해 조심스레 다가간다. 은근하고 섬세해 "
			 "안전하다고 느낄수록 더 진한 애정을 보인다.");
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

// ───────────────────────────────────────────────
// 트리 기반 유사도 (가중치: 깊을수록 절반)
//   깊이 d로 들어가는 간선 가중치 = DFS_EDGE_BASE >> (d-1), 최소 1
//   거리 = 두 잎에서 LCA까지 올라가며 만난 간선 가중치의 합
//   루트 가까운 차이(주도성)는 무겁게, 세분화(깊은 잎) 차이는 가볍게.
// ───────────────────────────────────────────────
#define DFS_EDGE_BASE 8 // 루트 직속 분기 가중치. 한 단계 깊어질수록 절반.

static int dfs_edge_weight(int depth) {
	if (depth < 1) {
		return 0;
	}
	int w = DFS_EDGE_BASE >> (depth - 1);
	return w < 1 ? 1 : w;
}

// 루트에서 code 잎까지의 노드 index 경로를 path[]에 채운다.
// 반환: 경로 길이(노드 수, 잎 depth+1). 못 찾으면 -1.
static int dfs_find_path(const DfsTree *t, int node, const char code[],
						 int *path, int depth) {
	if (node < 0 || node >= t->n_nodes || depth >= DFS_TREE_MAX_NODES) {
		return -1;
	}
	path[depth] = node;
	const DfsTreeNode *n = &t->nodes[node];
	if (n->is_leaf) {
		return (strcmp(n->code, code) == 0) ? depth + 1 : -1;
	}
	for (int i = 0; i < n->n_opt; i++) {
		int len = dfs_find_path(t, n->child[i], code, path, depth + 1);
		if (len > 0) {
			return len;
		}
	}
	return -1;
}

// node(깊이 depth) 아래 가장 무거운 루트→잎 경로 가중치 합.
static int dfs_max_weight_depth(const DfsTree *t, int node, int depth) {
	if (node < 0 || node >= t->n_nodes) {
		return 0;
	}
	const DfsTreeNode *n = &t->nodes[node];
	if (n->is_leaf) {
		int s = 0;
		for (int d = 1; d <= depth; d++) {
			s += dfs_edge_weight(d);
		}
		return s;
	}
	int best = 0;
	for (int i = 0; i < n->n_opt; i++) {
		int v = dfs_max_weight_depth(t, n->child[i], depth + 1);
		if (v > best) {
			best = v;
		}
	}
	return best;
}

// 트리 내 두 잎 코드 사이 가중 거리. 둘 중 하나라도 없으면 -1.
int dfs_tree_distance(const DfsTree *t, const char a[], const char b[]) {
	int pa[DFS_TREE_MAX_NODES], pb[DFS_TREE_MAX_NODES];
	int la = dfs_find_path(t, t->root, a, pa, 0);
	int lb = dfs_find_path(t, t->root, b, pb, 0);
	if (la <= 0 || lb <= 0) {
		return -1;
	}

	// 공통 접두 = 루트부터 같은 노드. 마지막 공통 노드가 LCA.
	int i = 0;
	while (i < la && i < lb && pa[i] == pb[i]) {
		i++;
	}
	int lca_depth = i - 1;
	int da = la - 1, db = lb - 1; // 각 잎의 깊이

	int dist = 0;
	for (int d = lca_depth + 1; d <= da; d++) {
		dist += dfs_edge_weight(d);
	}
	for (int d = lca_depth + 1; d <= db; d++) {
		dist += dfs_edge_weight(d);
	}
	return dist;
}

// 트리 내 두 유형 코드 사이 유사도(0~100). 코드를 못 찾으면 0.
int dfs_tree_similarity(const DfsTree *t, const char ideal[],
						const char other[]) {
	int dist = dfs_tree_distance(t, ideal, other);
	if (dist < 0) {
		return 0;
	}
	int max_dist = 2 * dfs_max_weight_depth(t, t->root, 0);
	if (max_dist <= 0) {
		return 0;
	}
	if (dist > max_dist) {
		dist = max_dist;
	}
	return (max_dist - dist) * 100 / max_dist;
}

// 애착 안정성 잎 4종(코드/이름)은 두 트리가 공유.
static void dfs_fill_attach_leaves(DfsTree *t) {
	dfs_leaf(t, 3, "AS", "햇살 항구",
			 "안정 애착. 친밀함과 독립을 둘 다 편하게 받아들이고, 솔직하게 "
			 "소통하며 신뢰를 잘 쌓는다. 곁에 있으면 마음이 놓인다.");
	dfs_leaf(t, 4, "AV", "바람 부는 절벽",
			 "회피 애착. 독립을 중시하고 지나친 밀착은 부담스러워 거리를 "
			 "둔다. 감정 표현이 절제돼 있어 속을 천천히 보여준다.");
	dfs_leaf(t, 5, "AX", "보름달 파도",
			 "불안 애착. 가까움을 강하게 원하고 자주 확인받고 싶어 한다. "
			 "작은 거리에도 민감하게 흔들리지만 애정은 깊다.");
	dfs_leaf(t, 6, "FA", "안갯속 미로",
			 "혼란(불안+회피) 애착. 사랑을 원하면서도 상처가 두려워 다가갔다 "
			 "물러서기를 반복한다. 안전함을 느끼면 비로소 마음을 연다.");
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

// 사랑의 언어 잎 4종 (말·마음 / 함께·행동의 2x2)
static void dfs_fill_lang_leaves(DfsTree *t) {
	dfs_leaf(t, 3, "LW", "다정한 한마디",
			 "인정·응원의 말로 사랑을 느낀다. '고마워, 사랑해' 같은 따뜻한 "
			 "말 한마디가 하루를 버티게 하는 큰 힘이 된다.");
	dfs_leaf(t, 4, "LT", "포옹의 온기",
			 "스킨십과 가까운 거리로 사랑을 느낀다. 손을 잡고 안아줄 때 "
			 "말보다 빠르게 마음이 채워진다.");
	dfs_leaf(t, 5, "LQ", "같이 걷는 길",
			 "함께 보내는 온전한 시간으로 사랑을 느낀다. 휴대폰을 내려놓고 "
			 "집중해서 같이 있는 순간을 가장 소중히 여긴다.");
	dfs_leaf(t, 6, "LA", "말없는 선물",
			 "챙겨주는 행동과 선물로 사랑을 느낀다. 말보다 행동으로 보여주는 "
			 "세심한 마음에 깊이 감동한다.");
}

void dfs_build_lang_self_tree(DfsTree *t) {
	memset(t, 0, sizeof(*t));
	strncpy(t->title, "사랑의 언어", sizeof(t->title) - 1);
	t->root = 0;
	t->n_nodes = 7;
	dfs_set_branch(&t->nodes[0], "사랑받는다고 느끼는 순간은 주로?",
				   "다정한 말과 따뜻한 표현을 들을 때", 1,
				   "함께 시간을 보내거나 챙김을 받을 때", 2);
	dfs_set_branch(&t->nodes[1], "그 표현은 어느 쪽이 더 와닿아?",
				   "고마워·사랑해 같은 인정과 응원의 말", 3,
				   "손잡기·포옹 같은 스킨십과 애정 표현", 4);
	dfs_set_branch(&t->nodes[2], "함께일 때 어느 쪽이 더 좋아?",
				   "오롯이 둘만의 시간을 보내는 것", 5,
				   "필요한 걸 챙겨주거나 선물해 주는 것", 6);
	dfs_fill_lang_leaves(t);
}

void dfs_build_lang_ideal_tree(DfsTree *t) {
	memset(t, 0, sizeof(*t));
	strncpy(t->title, "사랑의 언어", sizeof(t->title) - 1);
	t->root = 0;
	t->n_nodes = 7;
	dfs_set_branch(&t->nodes[0], "상대가 사랑을 이렇게 표현해 주면 좋겠어:",
				   "다정한 말과 따뜻한 표현으로", 1,
				   "함께하는 시간과 챙김으로", 2);
	dfs_set_branch(&t->nodes[1], "어떤 말이 더 듣고 싶어?",
				   "인정과 응원의 말", 3,
				   "스킨십과 애정 표현", 4);
	dfs_set_branch(&t->nodes[2], "어떤 챙김이 더 좋아?",
				   "둘만의 시간을 충분히 내주는 것", 5,
				   "필요한 걸 챙겨주고 선물해 주는 것", 6);
	dfs_fill_lang_leaves(t);
}

void dfs_build_self_survey(DfsSurvey *s) {
	memset(s, 0, sizeof(*s));
	strncpy(s->name, "내 연애 성향", sizeof(s->name) - 1);
	s->n_trees = 3;
	dfs_build_self_tree(&s->trees[0]);
	dfs_load_or_keep(&s->trees[0], "database/tree_self_0.json");
	dfs_build_attach_self_tree(&s->trees[1]);
	dfs_load_or_keep(&s->trees[1], "database/tree_self_1.json");
	dfs_build_lang_self_tree(&s->trees[2]);
	dfs_load_or_keep(&s->trees[2], "database/tree_self_2.json");
}

void dfs_build_ideal_survey(DfsSurvey *s) {
	memset(s, 0, sizeof(*s));
	strncpy(s->name, "내 이상형", sizeof(s->name) - 1);
	s->n_trees = 3;
	dfs_build_ideal_tree(&s->trees[0]);
	dfs_load_or_keep(&s->trees[0], "database/tree_ideal_0.json");
	dfs_build_attach_ideal_tree(&s->trees[1]);
	dfs_load_or_keep(&s->trees[1], "database/tree_ideal_1.json");
	dfs_build_lang_ideal_tree(&s->trees[2]);
	dfs_load_or_keep(&s->trees[2], "database/tree_ideal_2.json");
}

int dfs_find_leaf_by_code(const DfsTree *t, const char code[]) {
	for (int i = 0; i < t->n_nodes; i++) {
		if (t->nodes[i].is_leaf && strcmp(t->nodes[i].code, code) == 0) {
			return i;
		}
	}
	return -1;
}

// 잎을 분기로 확장하면서 두 하위 유형의 이름을 직접 지정한다.
// 코드는 자동(부모코드+"1"/"2"). name0/name1이 NULL이거나 빈 문자열이면
// 기존처럼 "부모이름 · 선택지"로 자동 생성한다.
int dfs_extend_leaf_named(DfsTree *t, int leaf_idx, const char question[],
						  const char opt0[], const char opt1[],
						  const char name0[], const char name1[],
						  const char desc0[], const char desc1[]) {
	if (leaf_idx < 0 || leaf_idx >= t->n_nodes) {
		return 0;
	}
	if (!t->nodes[leaf_idx].is_leaf) {
		return 0;
	}
	if (t->n_nodes + 2 > DFS_TREE_MAX_NODES) {
		return 0;
	}

	char pcode[MAX_TYPE_LEN], pname[DFS_NAME_LEN], pdesc[DFS_DESC_LEN];
	strncpy(pcode, t->nodes[leaf_idx].code, MAX_TYPE_LEN - 1);
	pcode[MAX_TYPE_LEN - 1] = '\0';
	strncpy(pname, t->nodes[leaf_idx].name, DFS_NAME_LEN - 1);
	pname[DFS_NAME_LEN - 1] = '\0';
	strncpy(pdesc, t->nodes[leaf_idx].desc, DFS_DESC_LEN - 1);
	pdesc[DFS_DESC_LEN - 1] = '\0';

	int c0 = t->n_nodes, c1 = t->n_nodes + 1;
	char code[MAX_TYPE_LEN], name[DFS_NAME_LEN];

	snprintf(code, sizeof(code), "%s1", pcode);
	if (name0 && name0[0]) {
		snprintf(name, sizeof(name), "%s", name0);
	} else {
		snprintf(name, sizeof(name), "%s · %s", pname, opt0);
	}
	dfs_leaf(t, c0, code, name, (desc0 && desc0[0]) ? desc0 : pdesc);
	t->nodes[c0].user_added = 1;

	snprintf(code, sizeof(code), "%s2", pcode);
	if (name1 && name1[0]) {
		snprintf(name, sizeof(name), "%s", name1);
	} else {
		snprintf(name, sizeof(name), "%s · %s", pname, opt1);
	}
	dfs_leaf(t, c1, code, name, (desc1 && desc1[0]) ? desc1 : pdesc);
	t->nodes[c1].user_added = 1;

	// 기존 잎을 분기 노드로 변환
	dfs_set_branch(&t->nodes[leaf_idx], question, opt0, c0, opt1, c1);
	t->nodes[leaf_idx].user_added = 1;

	t->n_nodes += 2;
	return 1;
}

// 이름·설명 미지정 버전(자동). 기존 호출부 하위호환용 래퍼.
int dfs_extend_leaf(DfsTree *t, int leaf_idx, const char question[],
					const char opt0[], const char opt1[]) {
	return dfs_extend_leaf_named(t, leaf_idx, question, opt0, opt1, NULL, NULL,
								 NULL, NULL);
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
		cJSON_AddStringToObject(o, "desc", n->desc);
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
			cJSON *desc = cJSON_GetObjectItem(o, "desc");
			if (cJSON_IsString(desc)) {
				strncpy(n->desc, desc->valuestring, DFS_DESC_LEN - 1);
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
