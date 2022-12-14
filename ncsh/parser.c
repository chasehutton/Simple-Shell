#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <sysexits.h>
#include "scanner.h"
#include "tree.h"
#include "executor.h"
#include "process_tree.h"
#include "globexp.h"

#define INITAL_TOKEN_COUNT 80
#define NCSH_ARG_COUNT 80

static Tree *L();
static Tree *E();
static int Eopt();
static Tree *T();
static Tree *Topt(Tree *);
static Tree *P();
static Tree *Popt(Tree *);
static Tree *C();
static void S(Tree *);
static void SH(Tree *);
static void R(Tree *t);

static Token **g_tokens;
static int g_current;
static int g_had_error;
static char *g_line_start;
static int g_num_tokens;

static Token *peek() {
   return g_tokens[g_current];
}

static Token *previous() {
   return g_tokens[g_current - 1];
}

static int is_at_end() {
   return peek()->type == TOKEN_EOF;
}

static Token *advance() {
   if (!is_at_end()) g_current++;
   return previous();
}

static int check(TokenType type) {
   if (is_at_end()) return 0;
   return peek()->type == type;
}

static void consume(TokenType type, const char *message) {
   if (check(type)) {
      advance();
      return;
   }

   advance();
   fprintf(stderr, "Parse Error: %s\n", message);
   fflush(stderr);
   g_had_error = 1;
}

static int match(TokenType type) {
   if (check(type)) {
      advance();
      return 1;
   }

   return 0;
}

static void free_tokens(Token **tokens) {
   int i = 0;
   while (tokens[i]->type != TOKEN_EOF) {
      free(tokens[i]);
      i++;
   }
   free(tokens[i]);
   free(tokens);
}

static void process_remaining() {
   if (g_current < g_num_tokens) {
      fprintf(stderr, "Unreachable Tokens: %s\n",
	      g_line_start + (g_tokens[g_current]->start - g_line_start));
      fflush(stderr);
   }
   free_tokens(g_tokens);
}

static void free_tree(Tree *t) {
   if (t) {
      if (t->conjunction == NONE) {
	 int i = 0;
	 while (t->argv[i]) {
	    free(t->argv[i]);
	    i++;
	 }
	 free(t->argv);
	 if (t->output) {
	    free(t->output);
	 }

	 if (t->input) {
	    free(t->input);     
	 }
      }
    
      free_tree(t->left);
      free_tree(t->right);
      free(t);
   }
}

/* 
 * Following methods are responsible for implenting the grammar.
 * Code is redundant in places to better show case how each function
 * maps to a grammar rule.
 */

static Tree *L() {
   return E();
}

static Tree *E() {
   Tree *t = calloc(1, sizeof(Tree));
   if (!t) {
      err(EX_OSERR, "Memory Allocation Failed.\n");
   }
   Tree *single;
   
   single = T();
   if ((t->conjunction = Eopt()) == NONE) {
      free(t);
      return single;
   } else {
      t->left = single;
      t->right = NULL;
      return t;
   }
}

static int Eopt() {
   if (match(TOKEN_AND_TERMINATOR) || match(TOKEN_SEMI_TERMINATOR)) {
      return previous()->type == TOKEN_AND_TERMINATOR ? ASYNC : SEMI;
   } else {
      return NONE;
   }
}

static Tree *T() {
   Tree *t = calloc(1, sizeof(Tree));
   if (!t) {
      err(EX_OSERR, "Memory Allocation Failed.\n");
   }
   Tree *single;

   t->left = P();
   t->right = Topt(t->left);

   single = t->right;
   free(t);
   return single;
}

static Tree *Topt(Tree *in) {
   Tree *t = calloc(1, sizeof(Tree));
   if (!t) {
      err(EX_OSERR, "Memory Allocation Failed.\n");
   }

   if (match(TOKEN_LAND) || match(TOKEN_LOR)
       || match(TOKEN_SEMICOLON) || match(TOKEN_AND)) {
      TokenType operator = previous()->type;
      t->conjunction = operator == TOKEN_LAND ? AND : (operator == TOKEN_LOR ? OR : (operator == TOKEN_SEMICOLON ? SEMI : ASYNC));
      t->left = in;
      t->right = P();
      return Topt(t);
   }

   free(t);
   return in;
}

static Tree *P() {
   Tree *t = calloc(1, sizeof(Tree));
   if (!t) {
      err(EX_OSERR, "Memory Allocation Failed.\n");
   }
   Tree *single;

   t->left = C();
   t->right = Popt(t->left);

   single = t->right;
   free(t);
   return single;
}

static Tree *Popt(Tree *in) {
   Tree *t = calloc(1, sizeof(Tree));
   if (!t) {
      err(EX_OSERR, "Memory Allocation Failed.\n");
   }

   if (match(TOKEN_PIPE)) {
      t->conjunction = PIPE;
      t->left = in;
      t->right = C();
      return Popt(t);
   }

   free(t);
   return in;
}

static Tree *C() {
   Tree *t = calloc(1, sizeof(Tree));
   if (!t) {
      err(EX_OSERR, "Memory Allocation Failed.\n");
   }

   SH(t);
   if (!t->left) {
      S(t);
   }

   return t;
}

static void SH(Tree *t) {
   if (match(TOKEN_LEFT_PAREN)) {
      t->left = E();
      consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
      t->right = NULL;
      t->conjunction = SUBSHELL;
   }
}

static void S(Tree *t) {
   int i = 0;
   int size = NCSH_ARG_COUNT;
   Token *t_token;
   char **files = get_files();

   t->conjunction = NONE;
   t->argv = calloc(size + 1, sizeof(char *));
   if (!t->argv) err(EX_OSERR, "Memory Allocation Failed.\n");
   
   while (match(TOKEN_STRING)) {
      t_token = previous();
      t->argv[i] = malloc(t_token->length + 1);
      if (!t->argv[i]) err(EX_OSERR, "Memory Allocation Failed.\n");
      strncpy(t->argv[i], t_token->start, t_token->length);
      t->argv[i][t_token->length] = '\0';
      if (is_glob(t->argv[i])) {
	 int matched = 0, k = 0;
	 char glob_pattern[1024];
	 strcpy(glob_pattern, t->argv[i]);
	 if (files) {
	    free(t->argv[i]);
	    while (files[k]) {
	       if (match_glob(files[k], glob_pattern)) {
		  matched = 1;
		  t->argv[i] = malloc(strlen(files[k]) + 1);
		  if (!t->argv[i]) err(EX_OSERR, "Memory Allocation Failed.\n");
		  strcpy(t->argv[i], files[k]);
		  i++;
	       }
	       k++;
	    }
	    if (!matched) {
	       t->argv[i] = malloc(strlen(glob_pattern) + 1);
	       if (!t->argv[i]) err(EX_OSERR, "Memory Allocation Failed.\n");
	       strcpy(t->argv[i], glob_pattern);
	       i++;
	    }
	 }
      } else {
	 i++;
      }
      if (i >= size) {
	 t->argv = realloc(t->argv, sizeof(char *) * size * 2 + 1);
	 if (!t->argv) err(EX_OSERR, "Memory Reallocation Failed.\n");
      }
   }

   // Enter panic mode
   if (!(t->argv[0])) {
      g_had_error = 1;
      if (!is_at_end()) {
	 fprintf(stderr, "Parse error: invalid NULL command before symbol %.*s near column %d.\n",
		 g_tokens[g_current]->length, g_line_start + (g_tokens[g_current]->start - g_line_start), g_tokens[g_current]->column);
      } else {
	 fprintf(stderr, "Parse error: invalid NULL command before EOF.\n");
      }
   }

   free_files(files);
   t->argv[i] = NULL;
   R(t);
}

static void R(Tree *t) {
   if (match(TOKEN_LEFT_ANGLE_BRACKET)) {
      if (match(TOKEN_STRING)) {
	 char *p;
	 Token *t_token;
	 t_token = previous();
	 t->input = malloc(t_token->length + 1);
	 if (!t->input) err(EX_OSERR, "Memory Allocation Failed./n");
	 p = t->input;
	 strncpy(p, t_token->start, t_token->length);
	 p += t_token->length;
	 *p = '\0';
      } else {
	 g_had_error = 1;
	 fprintf(stderr, "Expected file following redirect.\n");
         fflush(stderr);
      }
   }

   if (match(TOKEN_RIGHT_ANGLE_BRACKET)) {
      if (match(TOKEN_STRING)) {
	 char *p;
	 Token *t_token;
	 t_token = previous();
	 t->output = malloc(t_token->length + 1);
	 if (!t->output) err(EX_OSERR, "Memory Allocation Failed./n");
	 p = t->output;
	 strncpy(p, t_token->start, t_token->length);
	 p += t_token->length;
	 *p = '\0';
      } else {
	 g_had_error = 1;
	 fprintf(stderr, "Expected file following redirect.\n");
         fflush(stderr);
      }
   }
}

/* Main parse method */
int parse(char *source) {
   Tree *tree;
   int i = 0, status;

   g_num_tokens = INITAL_TOKEN_COUNT;
   g_line_start = source;
   g_current = 0;
   g_had_error = 0;
   g_tokens = calloc(g_num_tokens, sizeof(Token *));
   if (!g_tokens) {
      err(EX_OSERR, "Memory Allocation Failed.\n");
   }
   init_scanner(source);

   // Accumulate a list of tokens
   for (;;) {
      g_tokens[i] = scan_token();

      /* Enter panic mode and display error msg */
      if (g_tokens[i]->type == TOKEN_ERROR) {
	 g_had_error = 1;
	 fprintf(stderr, "Unexpeted character %.*s at column %d\n",
		 g_tokens[i]->length, g_tokens[i]->start, g_tokens[i]->column);
	 fflush(stderr);
      }
      if (g_tokens[i]->type == TOKEN_EOF) break;
      i++;

      if (i >= g_num_tokens) {
	 g_tokens = realloc(g_tokens, sizeof(Token *) * g_num_tokens * 2);
	 if (!g_tokens) err(EX_OSERR, "Memory Reallocation Failed.\n");
	 g_num_tokens *= 2;
      }
   }

#if defined(DEBUG_MODE)
#define ERR_MSGS 80
   char **err_msg = NULL;
   Tree *t = L();
   print_tree(t);
   err_msg = calloc(ERR_MSGS + 1, sizeof(char *));
   if (!err_msg) err(EX_OSERR, "Memory Allocation Failed.\n");
   process_tree(t, err_msg);
   if (err_msg[0]) {
      int i = 0;
      while (err_msg[i]) {
	 printf("%s\n", err_msg[i]);
	 i++;
      }
   }
   free_err_msgs(err_msg);
   free_tree(t);
   status = 0;
#else
   g_num_tokens = i;
   tree = L();
   // IF panic mode was triggered do not attempt to execute parse tree
   status = g_had_error ? 1 : execute(tree);
   process_remaining();
   free_tree(tree);
#endif
   return status;
}



