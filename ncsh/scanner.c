#include <string.h>
#include <ctype.h>
#include <err.h>
#include <sysexits.h>
#include <stdio.h>
#include <stdlib.h>
#include "scanner.h"

typedef struct {
   const char *start;
   const char *current;
} Scanner;

static Scanner scanner;
static int g_column;

void init_scanner(const char *source) {
   g_column = 0;
   scanner.start = source;
   scanner.current = source;
}

static int is_at_end() {
   return *scanner.current == '\0';
}


static char advance() {
   scanner.current++;
   return scanner.current[-1];
}

static char peek() {
   return *scanner.current;
}

static char peek_next() {
   if (is_at_end()) return '\0';
   return scanner.current[1];
}

static int is_operator(char c) {
   char cn = peek_next();
   return (c == '&' && cn == '&')
      || (c == '|' && cn == '|')
      || (c == '|')
      || (c == '(')
      || (c == ')')
      || (c == ';')
      || (c == '&')
      || (c == '<')
      || (c == '>');;
}

static int match (char expected) {
   if (is_at_end()) return 0;
   if (*scanner.current != expected) return 0    ;
   scanner.current++;
   return 1;
}

static Token *make_token(TokenType type) {
   Token *token = calloc(1, sizeof(Token));
   if (!token) err(EX_OSERR, "Memory Allocation Failed.\n");
   token->type = type;
   token->start = scanner.start;
   token->length = (int)(scanner.current - scanner.start);
   token->column = g_column;
   return token;
}

static void skip_white_space() {
   for (;;) {
      char c = peek();
      switch (c) {
      case ' ':
      case '\r':
      case '\t':
      case '\v':
      case '\f':
      case '\n':
	 advance();
	 break;
      default:
	 return;
      }
   }
}

static Token *string() {
   char c;
   while((c = peek()) != ' ' && !is_operator(c) && !is_at_end()) advance();
   return make_token(TOKEN_STRING);
}

Token *scan_token() {
   skip_white_space();
   g_column += (int)(scanner.current - scanner.start);
   scanner.start = scanner.current;

   if (is_at_end()) return make_token(TOKEN_EOF);
   char c = advance();
   char n;
   TokenType type;

   // Try to tokenize characters
   switch(c) {
   case '|':
      return make_token(
			match('|') ? TOKEN_LOR : TOKEN_PIPE);
   case '&':
      skip_white_space();
      if (is_at_end()) {
	 type = TOKEN_AND_TERMINATOR;
      } else if (match('&')) {
	 type = TOKEN_LAND;
      } else {
	 skip_white_space();
	 if ((n = peek()) == ')') {
	    type = TOKEN_AND_TERMINATOR;
	 } else {
	    type = TOKEN_AND;
	 }
      }
      return make_token(type);
   case '(': return make_token(TOKEN_LEFT_PAREN);
   case ')': return make_token(TOKEN_RIGHT_PAREN);
   case ';':
      skip_white_space();
      if (is_at_end()) {
	 type = TOKEN_SEMI_TERMINATOR;
      } else {
	 skip_white_space();
	 if ((n = peek()) == ')') {
	    type = TOKEN_SEMI_TERMINATOR;
	 } else {
	    type = TOKEN_SEMICOLON;
	 }
      }
      return make_token(type);
   case '<': return make_token(TOKEN_LEFT_ANGLE_BRACKET);
   case '>': return make_token(TOKEN_RIGHT_ANGLE_BRACKET);
   case '.':
   case '-':
   case '/':
   case '*':
   case '?':
   case '$':
   case '[':
      return string();
   }

   if (isalpha(c)) return string();
   if (isdigit(c)) return string();

   return make_token(TOKEN_ERROR);
}
  


