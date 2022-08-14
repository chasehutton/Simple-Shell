#if !defined(SCANNER_H)
#define SCANNER_H
#define TOKEN_BUF_LEN 80

typedef enum {
   // Single-character tokens.
  TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
  TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
  TOKEN_LEFT_ANGLE_BRACKET, TOKEN_RIGHT_ANGLE_BRACKET,
  TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
  TOKEN_SLASH, TOKEN_STAR, TOKEN_UNDERSCORE,

  // Operator tokens
  TOKEN_PIPE, TOKEN_LAND, TOKEN_LOR, TOKEN_AND,
  TOKEN_SEMICOLON, TOKEN_AND_TERMINATOR, TOKEN_SEMI_TERMINATOR,

  // Literals
  TOKEN_STRING,

  // Other
  TOKEN_EOF, TOKEN_ERROR
} TokenType;

typedef struct {
  TokenType type;
  const char *start;
  int length;
  int column;
} Token;

void init_scanner(const char *);
Token *scan_token();
#endif
