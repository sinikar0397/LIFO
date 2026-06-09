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
 * Date Fit Survey 결과 코드는 트리의 경로를 저장한다.
 * 예: DTF = 지배형 + 이성적 리드 + 정면 돌파
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

int compat(People *a, People *b);

#endif // DFS_H
