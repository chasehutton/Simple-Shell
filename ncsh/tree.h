#if !defined(TREE_H)
#define TREE_H

typedef struct tree {
  enum { NONE = 0, AND, OR, PIPE, SUBSHELL, ASYNC, SEMI } conjunction;
  struct tree *right;
  struct tree *left;
  char **argv;
  char *output;
  char *input;
} Tree;

static const char *conj[] __attribute__((unused)) = { "err", "&&", "||", "|", "()", "&", ";"};

int parse(char *);
#endif
