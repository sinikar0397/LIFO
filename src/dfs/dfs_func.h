#ifndef DFS_FUNC_H
#define DFS_FUNC_H

#include "dfs.h"

// int dfs_read_int_in_range(int min_value, int max_value);

void dfs_set_branch(DfsTreeNode *n, const char *q, const char *o0, int c0, const char *o1, int c1);

void dfs_set_leaf(DfsTreeNode *n, const char *code, const char *name);

void dfs_load_or_keep(DfsTree *t, const char *path);

#endif