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
// void dfs_run_date_fit_survey(char result_type[MAX_TYPE_LEN], const char title[]);
// void dfs_customize_date_fit_type(char result_type[MAX_TYPE_LEN]);

DateFitNodeId dfs_find_type_node(const char type[]);
int dfs_depth(DateFitNodeId node);
DateFitNodeId dfs_lca(DateFitNodeId u, DateFitNodeId v);
int dfs_distance(DateFitNodeId u, DateFitNodeId v);
int dfs_type_distance(const char type_a[], const char type_b[]);
int dfs_type_similarity(const char ideal_type[], const char other_type[]);
void dfs_print_type_description(const char type[]);
// 유형 코드(예: "DTF")에 해당하는 유형명을 돌려준다. 못 찾으면 루트명을 반환.
const char *dfs_type_name(const char type[]);

// 애착 유형(AS/AV/AX/FA) 사이의 유사도(0~100)와 유형명.
int dfs_attach_similarity(const char ideal_type[], const char other_type[]);
const char *dfs_attach_name(const char code[]);

int compat(People *a, People *b);

// 매칭용 공용 트리 캐시를 무효화한다(세분화로 트리 파일이 바뀐 뒤 호출).
void dfs_matching_reload(void);

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
	char save_path[64];					// 이 트리를 저장/로드할 파일 경로
} DfsTree;

// 한 설문(성향/이상형)은 대주제별 트리 여러 개로 구성된다.
#define DFS_MAX_TREES 4

typedef struct DfsSurvey {
	char name[48];				   // 설문 이름 ("내 연애 성향" 등)
	DfsTree trees[DFS_MAX_TREES];  // 대주제별 트리
	int n_trees;
} DfsSurvey;

// 대주제 트리들 (성향/이상형 각각)
void dfs_build_self_tree(DfsTree *tree);		// 주도·표현 성향
void dfs_build_ideal_tree(DfsTree *tree);
void dfs_build_attach_self_tree(DfsTree *tree); // 애착 안정성
void dfs_build_attach_ideal_tree(DfsTree *tree);

// "내 성향" / "내 이상형" 설문(대주제 트리 묶음)을 만든다.
void dfs_build_self_survey(DfsSurvey *survey);
void dfs_build_ideal_survey(DfsSurvey *survey);

// 루트에서 잎까지의 최대 질문 수.
int dfs_tree_max_depth(const DfsTree *tree);

// 트리 내 두 유형 코드 사이의 가중 거리/유사도(0~100).
// 깊을수록(세분화일수록) 간선 가중치가 작아 차이가 점수에 덜 반영된다.
// 세분화로 추가된 유형 코드도 그대로 계산되므로 동적 트리를 그대로 지원한다.
int dfs_tree_distance(const DfsTree *tree, const char a[], const char b[]);
int dfs_tree_similarity(const DfsTree *tree, const char ideal[],
						const char other[]);

// 코드로 잎 노드 index를 찾는다. 없으면 -1.
int dfs_find_leaf_by_code(const DfsTree *tree, const char code[]);

// 잎(leaf_idx)을 분기 노드로 확장한다. 새 하위 유형 2개를 잎으로 추가하며,
// 하위 유형명은 "부모유형명 · 선택지", 코드는 "부모코드1"/"부모코드2"로 자동 생성.
// 성공 시 1 (공간 부족/잘못된 노드면 0).
int dfs_extend_leaf(DfsTree *tree, int leaf_idx, const char question[],
					const char opt0[], const char opt1[]);

// 트리를 JSON 파일로 저장/로드. 로드는 파일이 있으면 1(트리 덮어씀), 없으면 0.
int dfs_save_tree(const DfsTree *tree, const char path[]);
int dfs_load_tree(DfsTree *tree, const char path[]);

#endif // DFS_H
