#ifndef scute_scanner_h
#define scute_scanner_h

typedef enum {
    TK_PLUS, TK_MINUS, TK_TIMES, TK_DIVIDE, TK_MODULO, TK_ASSIGN, TK_EQUALS,
    TK_INCR_ASSIGN, TK_DECR_ASSIGN,
    TK_BANG, TK_BANG_EQUALS, TK_INCR, 
    TK_DECR, TK_COLON, TK_QUESTION, TK_LESS_EQUALS,
    TK_GREATER_EQUALS, TK_LESS, TK_GREATER, TK_EVAL_ASSIGN,
    TK_L_LIMIT, TK_R_LIMIT,

    TK_REAL,
    TK_INTEGER,
    TK_TRUE,
    TK_FALSE,
    TK_NULL,
    TK_STRING,
    TK_ID,
    TK_FUNC,

    TK_AND,
    TK_OR,
    TK_PRE,

    TK_PI, TK_E, TK_TAU,
 
    TK_SEMI, TK_L_BRACE, TK_R_BRACE, TK_L_PAREN, TK_R_PAREN, 
    TK_L_BRACK, TK_R_BRACK, TK_COMMA, TK_DEREF, TK_TILDA, TK_NEWLINE,
    TK_INDENT,

    TK_DO,
    TK_WHILE,
    TK_FOR,
    TK_IF,
    TK_ELSE,
    TK_RECT,
    TK_CIRC,
    TK_ELLIP,
    TK_LET,
    TK_PRINT,
    TK_DRAW,
    TK_TEXT,
    TK_T,

    TK_ERROR,
    TK_EOF

} TKType;

typedef struct{
    TKType type;
    const char* start;
    int length;
    int line;
    int indent;
} TK; 

void initScanner(const char* source);
TK scanTK();

#endif