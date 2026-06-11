#ifndef DFS_H
#define DFS_H

#include "../headers.h"
#include "../people/people.h"

typedef enum DateFitNodeId {
	DFS_NODE_ROOT,
	DFS_NODE_DOMINANT,
	DFS_NODE_SUBMISSIVE,
	DFS_NODE_DOMINANT_THINKING,
	DFS_NODE_DOMINANT_EMOTIONAL,
	DFS_NODE_SUBMISSIVE_THINKING,
	DFS_NODE_SUBMISSIVE_EMOTIONAL,
	DFS_NODE_DTF,
	DFS_NODE_DTC,
	DFS_NODE_DEF,
	DFS_NODE_DEC,
	DFS_NODE_STF,
	DFS_NODE_STC,
	DFS_NODE_SEF,
	DFS_NODE_SEC,
	DFS_NODE_COUNT
} DateFitNodeId;

/*
 * Date Fit Survey 결과 코드?�� ?��리의 경로�? ????��?��?��.
 * ?��: DTF = �?배형 + ?��?��?�� 리드 + ?���? ?��?��
 */
void dfs_run_date_fit_survey(char result_type[MAX_TYPE_LEN], const char title[]);
void dfs_customize_date_fit_type(char result_type[MAX_TYPE_LEN]);

DateFitNodeId dfs_find_type_node(const char type[]);
int dfs_depth(DateFitNodeId node);
DateFitNodeId dfs_lca(DateFitNodeId u, DateFitNodeId v);
int dfs_distance(DateFitNodeId u, DateFitNodeId v);
int dfs_type_distance(const char type_a[], const char type_b[]);
int dfs_type_similarity(const char ideal_type[], const char other_type[]);
void dfs_print_type_description(const char type[]);
// 유형 코드(예: "DTF")에 해당하는 유형명을 돌려준다. 못 찾으면 루트명을 반환.
const char *dfs_type_name(const char type[]);

int compat(People *a, People *b);

// ───────────────────────────────────────────────
// 트리 엔진 (GUI 설문용)
// 하드코딩 분기 대신, 노드 배열로 트리를 표현한다.
// 내부 노드는 질문 + 선택지, 잎 노드는 유형 코드/이름을 가진다.
// 사용자가 잎 아래로 질문을 추가(유형 세분화)하는 것도 같은 구조로 처리한다.
// ───────────────────────────────────────────────
#define DFS_MAX_OPT 4		   // 한 질문의 최대 선택지 수
#define DFS_TREE_MAX_NODES 64  // 한 트리의 최대 노드 수
#define DFS_Q_LEN 200		   // 질문 문자열 최대 길이
#define DFS_OPT_LEN 100		   // 선택지 문자열 최대 길이
#define DFS_NAME_LEN 48		   // 유형명 최대 길이

typedef struct DfsTreeNode {
	int is_leaf;					   // 잎이면 1
	char question[DFS_Q_LEN];		   // 내부 노드: 분기 질문
	int n_opt;						   // 선택지 개수
	char opt_text[DFS_MAX_OPT][DFS_OPT_LEN]; // 선택지 텍스트
	int child[DFS_MAX_OPT];			   // 선택지 i가 가리키는 자식 노드 index
	char code[MAX_TYPE_LEN];		   // 잎: 유형 코드 (예: DTF)
	char name[DFS_NAME_LEN];		   // 잎: 유형명
	int user_added;					   // 사용자가 추가한 노드면 1 (Phase 3용)
} DfsTreeNode;

typedef struct DfsTree {
	char title[64];						// 대주제 이름
	DfsTreeNode nodes[DFS_TREE_MAX_NODES];
	int n_nodes;
	int root;							// 루트 노드 index
} DfsTree;

// "내 성향" 진단 기본 트리를 만든다.
void dfs_build_self_tree(DfsTree *tree);
// "내 이상형" 진단 기본 트리를 만든다. (구조/코드는 같고 질문 표현만 다름)
void dfs_build_ideal_tree(DfsTree *tree);
// 루트에서 잎까지의 최대 질문 수(스텝 수 계산용).
int dfs_tree_max_depth(const DfsTree *tree);

#endif // DFS_H
