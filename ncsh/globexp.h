#if !defined(GLOB_EXP_H)
#define GLOB_EXP_H
char **get_files();
int match_glob(const char *, const char *);
int is_glob(char *);
void free_files(char **);
#endif
