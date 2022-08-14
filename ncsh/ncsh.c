#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <err.h>
#include <errno.h>
#include <sysexits.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <pwd.h>
#include "tree.h"

#define PROMPT_SIZE 2048

/*
 * Generates prompt in bash format:
 * user_name@host_name:current_working_directory$ 
 * if this function fails for any reason default prompt of "ncsh$ "
 * is returned.
 */
static char *get_prompt() {
   char *prompt = calloc(1, PROMPT_SIZE);
   if (!prompt) err(EX_OSERR, "Memory Allocation Failed.\n");
   struct passwd *p;
   uid_t uid = getuid();

   if (!(p = getpwuid(uid))) {
      strcpy(prompt, "ncsh$ ");
      return prompt;
   } else {
      char *prompt_ptr = prompt;
      int l = strlen(p->pw_name);
      strncpy(prompt_ptr, p->pw_name, l);
      prompt_ptr += l;
      *prompt_ptr++ = '@';
      if (gethostname(prompt_ptr, PROMPT_SIZE) == -1) {
	 strcpy(prompt, "ncsh$ ");
	 return prompt;
      }
      prompt_ptr += ((prompt + strlen(prompt)) - prompt_ptr); 
      *prompt_ptr++ = ':';
      if (!getcwd(prompt_ptr, PROMPT_SIZE)) {
	 strcpy(prompt, "ncsh$ ");
	 return prompt;
      }
      prompt_ptr += ((prompt + strlen(prompt)) - prompt_ptr);
      *prompt_ptr++ = '$';
      *prompt_ptr++ = ' ';
      *prompt_ptr = '\0';
      return prompt;
   }
}

/*
 * The following is an implementation of a simplified bash shell.
 * This projet contains methods for scanning, parsing, and executig
 * user input at the command line. Each line of text will be scanned in
 * which characters will be tokenized. The list of tokens will then be
 * parsed into a tree according to a simplified grammar dervived from 
 * Bash's grammar. Finally the parse tree will be executed and its status
 * returned.
 */
int main() {
   char *buffer;
   int status;
   char *prompt = get_prompt();
   
   // Main Loop
   while ((buffer = readline(prompt))) {
      if (buffer[0] == 0) { free(buffer); continue; }
      add_history(buffer);
      status = parse(buffer);
      free(buffer);
      free(prompt);
      prompt = get_prompt();
   }
   exit(status);
}


