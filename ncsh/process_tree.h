#if !defined(PROCESS_TREE_H)
#define PROCESS_TREE_H

#include "tree.h"
#define TREE_SUCCESS 1
#define TREE_ERROR 0

void process_tree(Tree *, char **);
void print_tree(Tree *);
void free_err_msgs(char **);
#endif
