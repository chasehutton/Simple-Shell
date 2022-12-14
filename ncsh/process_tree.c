#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <sysexits.h>
#include <string.h>
#include "process_tree.h"

static int i;
/*
 * Provides Aditional Debuging funtionality.
 * Methods will only be avliable in DEBUG MODE
 */
void process_tree(Tree *t, char **err_msg) {
   if (t) {
      process_tree(t->left, err_msg);
      process_tree(t->right, err_msg);
      // List Terminator
      if (t->conjunction == SEMI || t->conjunction == ASYNC) {
	 if (t->right && t->right->conjunction == NONE
	     && !(t->right->argv[0])) {
	    t->right = NULL;
	 }
      } else if (t->conjunction == AND || t->conjunction == OR
		 || t->conjunction == PIPE) {
	 char buf[1024];
	 if (t->left && t->left->conjunction == NONE &&
	     !(t->left->argv[0])) {
	    sprintf(buf, "Invalid NULL Command. Expected left operand of symbol '%s'",
		    conj[t->conjunction]);
	    err_msg[i] = malloc(strlen(buf) + 1);
	    if (!err_msg[i]) err(EX_OSERR, "Memory Allocation Failed.\n");
	    strcpy(err_msg[i++], buf);
	 } else if (t->right && t->right->conjunction == NONE &&
		    !(t->right->argv[0])) {
	    sprintf(buf, "Invalid NULL Command. Expected right operand of symbol '%s'",
		    conj[t->conjunction]);
	    err_msg[i] = malloc(strlen(buf) + 1);
	    if (!err_msg[i]) err(EX_OSERR, "Memory Allocation Failed.\n");
	    strcpy(err_msg[i++], buf);
	 }
      }
   }
}

static void print_tree_aux(Tree *t, int space) {
   int i, y = 0;
   char buf[1024];
   char *buf_ptr = &buf[0];
   if (!t) return;
   space += 10;
   print_tree_aux(t->right, space);
   printf("\n");
   for (i = 10; i < space; i++) printf(" ");
   if (t->conjunction == NONE) {
      while (t->argv[y]) {
	 strcpy(buf_ptr, t->argv[y]);
	 buf_ptr += strlen(t->argv[y]);
	 *buf_ptr++ = ' ';
	 y++;
      }

      *buf_ptr = '\0';
      printf("NONE: %s", buf);

      if (t->input) {
         printf(", (IR) %s", t->input);
      }

      if (t->output) {
         printf(", (OR) %s", t->output);
      }

      printf("\n");
   } else {
      printf("%s\n", conj[t->conjunction]);
   }
   print_tree_aux(t->left, space);
}

void print_tree(Tree *t) {
   print_tree_aux(t, 0);
}

void free_err_msgs(char **err_msgs) {
   int i = 0;
   while (err_msgs[i]) {
      free(err_msgs[i]);
      i++;
   }
   free(err_msgs);
}
