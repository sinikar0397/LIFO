#ifndef DFS_FUNC_H
#define DFS_FUNC_H

#include "dfs.h"

// 내부(분기) 노드를 채우는 헬퍼: 2지선다용.
void dfs_set_branch(DfsTreeNode *n, const char *q, const char *o0, int c0, const char *o1, int c1);

// 잎 노드를 채우는 헬퍼.
void dfs_set_leaf(DfsTreeNode *n, const char *code, const char *name);

// 기본 트리를 빌드한 뒤, 저장 파일이 있으면 덮어쓰고, 마지막에 경로를 박아둔다.
void dfs_load_or_keep(DfsTree *t, const char *path);

#endif