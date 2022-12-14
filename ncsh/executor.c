#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sysexits.h>
#include <err.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "tree.h"
#include "executor.h"

#define OPEN_FLAGS (O_WRONLY | O_TRUNC | O_CREAT)
#define DEF_MODE 0664

static int execute_aux(struct tree *, int, int);
static struct sigaction sa;
static struct sigaction da;

static void handle_sigchld_donothing() {
   return;
}

static void handle_sigchld_async(int sig) {
   int saved_errno = errno;
   while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
   errno = saved_errno;
}

int execute(Tree *t) {
   sa.sa_handler = &handle_sigchld_async;
   sigemptyset(&sa.sa_mask);
   sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;

   da.sa_handler = &handle_sigchld_donothing;
   sigemptyset(&da.sa_mask);
   da.sa_flags = SA_RESTART | SA_NOCLDSTOP;

   if (sigaction(SIGCHLD, &sa, 0) == -1) {
      perror(0);
      exit(1);
   }
   return execute_aux(t, STDIN_FILENO, STDOUT_FILENO);
}

static int execute_aux(Tree *t, int p_input_fd, int p_output_fd) {
   if (t) {
      if (t->conjunction == NONE) {
	 int status = 0;
	 if (sigaction(SIGCHLD, &da, 0) == -1) {
	    perror(0);
	    exit(1);
	 }
	 if (!strcmp("cd", t->argv[0])) {
	    char *path = t->argv[1] ? t->argv[2] ? NULL : t->argv[1] : getenv("HOME");
	    if (path) {
	       if (chdir(path) == -1) {
		  perror(path);
		  status = 1;
	       }
	    } else {
	       perror("Too many arguments");
	       status = 1;
	    }
	 } else if (!strcmp("exit", t->argv[0])) {
	    if (t->argv[1]) {
	       perror("Too many arguments");
	       status = 1;
	    } else {
	       exit(EXIT_SUCCESS);
	    }
	 } else {
	    pid_t pid;
	    int input_fd, output_fd;

	    if ((pid = fork()) < 0) {
	       perror("Fork failed");
	       exit(EX_OSERR);
	    }

	    if (!pid) {
	       input_fd = t->input ? open(t->input, O_RDONLY, DEF_MODE) : p_input_fd;
	       if (input_fd == -1) {
		  perror("Failed to open file");
		  exit(EX_OSERR);
	       }

	       output_fd = t->output ? open(t->output, OPEN_FLAGS, DEF_MODE) : p_output_fd;
	       if (output_fd == -1) {
		  perror("Failed to open file");
		  exit(EX_OSERR);
	       }

	       if (dup2(input_fd, STDIN_FILENO) == -1) {
		  perror("");
		  exit(EX_OSERR);
	       }

	       if (input_fd != STDIN_FILENO && close(input_fd) == -1) {
		  perror("");
		  exit(EX_OSERR);
	       }

	       if (dup2(output_fd, STDOUT_FILENO) == -1) {
		  perror("");
		  exit(EX_OSERR);
	       }

	       if (output_fd != STDOUT_FILENO && close(output_fd) == -1) {
		  perror("");
		  exit(EX_OSERR);
	       }

	       execvp(t->argv[0], t->argv);
	       fprintf(stderr, "Failed to execute %s\n", t->argv[0]);
	       fflush(stdout);
	       exit(EX_OSERR);
	    } else {
	       wait(&status);
	       if (sigaction(SIGCHLD, &sa, 0) == -1) {
		  perror(0);
		  exit(1);
	       }
	       if (status) {
		  status = 1;
	       }
	    }
	 }
	 return status;
      } else if (t->conjunction == AND) {
	 int status;
	 if ((status = execute_aux(t->left, p_input_fd, p_output_fd)) == 0) {
	    status = execute_aux(t->right, p_input_fd, p_output_fd); 
	 }
	 
	 return status;
      } else if (t->conjunction == PIPE) {	 
	 int pipefd[2], input_fd, output_fd, left_status, right_status;
	 pid_t pid_left, pid_right;

	 if (sigaction(SIGCHLD, &da, 0) == -1) {
	    perror(0);
	    exit(1);
         }
	 
	 if (t->left && t->left->output) {
	    fprintf(stderr, "Ambiguous output redirect.\n");
	    return 1;
	 }

	 if (t->right && t->right->input) {
	    fprintf(stderr, "Ambiguous input redirect.\n");
	    return 1;
	 }
	 
	 if (pipe(pipefd) == -1) {
	    perror("");
	    exit(EX_OSERR);
	 }

	 if ((pid_left = fork()) < 0) {
	    perror("Fork failed");
	    exit(EX_OSERR);
	 }

	 if (pid_left == 0) {
	    if (close(pipefd[0]) == -1) {
	       perror("");
	       exit(EX_OSERR);
	    }

	    input_fd = t->input ? open(t->input, O_RDONLY, DEF_MODE) : p_input_fd;
	    if (input_fd == -1) {
	       perror("Failed to open file");
	       exit(EX_OSERR);
	    }
	    
	    left_status = execute_aux(t->left, input_fd, pipefd[1]);
	    	    
	    if (close(pipefd[1]) == -1) {
	       perror("");
	       exit(EX_OSERR);
	    }
	    
	    exit(left_status);
	 } else {
	    if ((pid_right = fork()) < 0) {
	       perror("Fork failed");
	       exit(EX_OSERR);
	    }

	    if (pid_right == 0) {
	       if (close(pipefd[1]) == -1) {
		  perror("");
		  exit(EX_OSERR);
	       }

	       output_fd = t->output ? open(t->output, OPEN_FLAGS, DEF_MODE) : p_output_fd;
	       if (output_fd == -1) {
		  perror("Failed to open file");
		  exit(EX_OSERR);
	       }

	       right_status = execute_aux(t->right, pipefd[0], output_fd);
	       
	       if (close(pipefd[0]) == -1) {
		  perror("");
		  exit(EX_OSERR);
	       }
	   
	       exit(right_status);
	    } else {
	       if (close(pipefd[0]) == -1) {
		  perror("");
		  exit(EX_OSERR);
	       }

	       if (close(pipefd[1]) == -1) {
		  perror("");
		  exit(EX_OSERR);
	       }
	              
	       wait(&left_status);
 	       wait(&right_status);

	       if (sigaction(SIGCHLD, &sa, 0) == -1) {
		  perror(0);
		  exit(1);
	       }
	       if (left_status || right_status) {
		  return 1;
	       }	
	    }
	 }
      } else if (t->conjunction == SUBSHELL) {
	 pid_t pid;
	 int status, input_fd, output_fd;

	 if (sigaction(SIGCHLD, &da, 0) == -1) {
	    perror(0);
	    exit(1);
	 }
       
	 if ((pid = fork()) < 0) {
	    perror("Fork failed");
	    exit(EX_OSERR);
	 }

	 if (!pid) {
	    input_fd = t->input ? open(t->input, O_RDONLY, DEF_MODE) : p_input_fd;
	    if (input_fd == -1) {
	       perror("Failed to open file");
	       exit(EX_OSERR);
	    }
	    output_fd = t->output ? open(t->output, OPEN_FLAGS, DEF_MODE) : p_output_fd;
	    if (output_fd == -1) {
	       perror("Failed to open file");
	       exit(EX_OSERR);
	    }
	    status = execute_aux(t->left, input_fd, output_fd);
	    exit(status);
	 } else {
	    wait(&status);
	    if (sigaction(SIGCHLD, &sa, 0) == -1) {
	       perror(0);
	       exit(1);
	    }
	    if (status) {
	       return 1;
	    }
	 }
      } else if (t->conjunction == OR) {
	 int status;

	 if ((status = execute_aux(t->left, p_input_fd, p_output_fd)) == 1) {
	    status = execute_aux(t->right, p_input_fd, p_output_fd);
	 }

	 return status;
      } else if (t->conjunction == SEMI) {
	 int status;

	 status = execute_aux(t->left, p_input_fd, p_output_fd);
	 if (t->right) status = execute_aux(t->right, p_input_fd, p_output_fd);

	 return status;
      } else if (t->conjunction == ASYNC) {
	 pid_t pid;
	 int status;
       
	 if ((pid = fork()) < 0) {
	    perror("Fork failed");
	    exit(EX_OSERR);
	 }

	 if (!pid) {
	    status = execute_aux(t->left, p_input_fd, p_output_fd);
	    exit(status);
	 } else {
	    if (t->right) {
	       status = execute_aux(t->right, p_input_fd, p_output_fd);
	       return status;
	    }
	    return 0;
	 }
      }
      return 0;
   }
   return 1;
}
