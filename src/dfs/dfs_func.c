#include "dfs_func.h"

#include <stdio.h>
#include <string.h>

static void dfs_copy_text(char *dst, size_t dst_size, const char *src) {
	if (dst_size == 0) {
		return;
	}
	if (src == NULL) {
		dst[0] = '\0';
		return;
	}

	strncpy(dst, src, dst_size - 1);
	dst[dst_size - 1] = '\0';
}

void dfs_set_branch(DfsTreeNode *n, const char *q, const char *o0,
					int c0, const char *o1, int c1) {
	n->is_leaf = 0;
	n->user_added = 0;
	n->n_opt = 2;
	dfs_copy_text(n->question, sizeof(n->question), q);
	dfs_copy_text(n->opt_text[0], sizeof(n->opt_text[0]), o0);
	n->child[0] = c0;
	dfs_copy_text(n->opt_text[1], sizeof(n->opt_text[1]), o1);
	n->child[1] = c1;
}

void dfs_set_leaf(DfsTreeNode *n, const char *code, const char *name) {
	n->is_leaf = 1;
	n->user_added = 0;
	n->n_opt = 0;
	dfs_copy_text(n->code, sizeof(n->code), code);
	dfs_copy_text(n->name, sizeof(n->name), name);
}

void dfs_load_or_keep(DfsTree *t, const char *path) {
	dfs_load_tree(t, path);
	dfs_copy_text(t->save_path, sizeof(t->save_path), path);
}