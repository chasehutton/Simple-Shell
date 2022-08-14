#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <sysexits.h>
#include <unistd.h>
#include "tinydir.h"

#define INITAL_FILE_COUNT 80
char **get_files() {
   char **files = calloc(INITAL_FILE_COUNT + 1, sizeof(char *));
   int i = 0, size = INITAL_FILE_COUNT;
   if (!files) { err(EX_OSERR, "Memory Allocation Failed.\n"); }
   char base_path[1024];
   if (!getcwd(base_path, 1024)) { return NULL; };
   tinydir_dir dir;
   tinydir_open(&dir, base_path);

   while (dir.has_next) {
      tinydir_file file;
      tinydir_readfile(&dir, &file);
      if (!strcmp(file.name, "..")) { tinydir_next(&dir); continue; }
      files[i] = malloc(strlen(file.name) + 1 + (file.is_dir ? 1 : 0));
      if (!files[i]) { err(EX_OSERR, "Memory Allocation Failed.\n"); }
      strcpy(files[i], file.name);
      if (file.is_dir) {
	 strcat(files[i], "/");
      }
      i++;
      if (i == size) {
	 files = realloc(files, sizeof(char *) * size * 2 + 1);
	 if (!files) err(EX_OSERR, "Memory Reallocation Failed.\n");
	 size *= 2;
      }
      tinydir_next(&dir);
   }
   files[i] = NULL;
   tinydir_close(&dir);
   return files;
}

void free_files(char **files) {
   int i = 0;
   while (files[i]) {
      free(files[i]);
      i++;
   }
   free(files[i]);
   free(files);
}

int is_glob(char *s) {
   while (*s) {
      if (*s == '?' || *s == '*' || *s == '[') return 1;
      s++;
   }
   return 0;
}

/*
 * oh god this is ugly
 * Attempt to expand glob pattern and match to input string.
 */
int match_glob(const char *s, const char *p) {
   const char* star = NULL;
   const char* ss = s;
   while (*s) {
      if ((*p == '?') || (*p == *s)) {
	 s++;
	 p++;
	 continue;
      }

      // TODO fix double count of first character
      if (*p == '[') {
	 const char *bp = NULL;
	 char *r = calloc(1, 1024);
	 char *rp;
	 int inversed = 0;
	 rp = r;
	 bp = p + 1;
	 while (*bp && *bp != ']') {
	    if (*bp == '!') {
	       inversed = 1;
	       bp++;
	       continue;
	    }
	    *rp = *bp;
	    if (*bp == '-') {
	       int i = (int)(bp[-1]);
	       for (; i < (int)(bp[1]); i++) {
		  *rp = (char)i;
		  rp++;
	       }
	       bp++;
	       continue;
	    }
	    rp++;
	    bp++;
	 }
	 if (!*bp) { return 0; }
	 *rp = '\0';

	 if (inversed) {
	    if (!*r) { return 0; }
	    char *ir = calloc(1, 1024);
	    char *irp = ir;
	    rp = r;
	    int i = 48, k;
	    for (; i < 122; i++) {
	       if (i == 58) { i = 64; continue; }
	       if (i == 91) { i = 96; continue; }

	       k = 0;
	       while (rp[k]) {
		  if ((char)i == rp[k]) {
		     break;
		  }
		  k++;
	       }

	       if (!rp[k]) { *irp++ = (char)i; }	       
	    }
	    *irp = '\0';
	    if (!*ir) { return 0; }
	    r = ir;
	 }
	 rp = r;
	 while (*rp) {
	    if (*s == *rp) {
	       s++;
	       p = bp + 1;
	       break;
	    }
	    rp++;
	 }

	 if (!*rp) {
            free(r);
            if (star) { s++; continue; }
            return 0;
         }	 
	 free(r);
	 continue;
      }
      
      if (*p == '*') {
	 star = p++;
	 ss = s;
	 continue;
      }
      
      if (star) {
	 p = star+1;
	 s = ++ss;
	 continue;
      } 
      return 0;
   }
   
   while (*p == '*'){ p++; }

   return !*p;  
}

