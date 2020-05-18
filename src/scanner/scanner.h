#ifndef scute_scanner_h
#define scute_scanner_h

typedef enum {
    TK_PLUS, 
    TK_MINUS,
    TK_TIMES,
    TK_DIVIDE,
    TK_MODULO,
    
    TK_EQUALS,
    TK_BANG_EQUALS, 
    TK_LESS_EQUALS,
    TK_GREATER_EQUALS, 
    TK_LESS, 
    TK_GREATER,

    TK_ASSIGN, 
    TK_INCR_ASSIGN,
    TK_DECR_ASSIGN,

    TK_BANG, TK_INCR, 
    TK_DECR, TK_COLON, TK_QUESTION, TK_EVAL_ASSIGN,
    TK_L_LIMIT, TK_R_LIMIT,

    TK_REAL,   
    TK_INTEGER,
    TK_TRUE,
    TK_FALSE,
    TK_NULL,
    TK_STRING,
    TK_ID,
	TK_CONST,
    TK_FUNC,
    TK_AND,
    TK_OR,
    TK_PRE,
 
    TK_SHAPE,
    TK_NATIVE,

    TK_SEMI, TK_L_BRACE, TK_R_BRACE, TK_L_PAREN, TK_R_PAREN, 
    TK_L_BRACK, TK_R_BRACK, TK_COMMA, TK_DEREF, TK_TILDA, TK_NEWLINE,
    TK_INDENT,

    TK_DO,
    TK_WHILE,
    TK_FOR,
    TK_IF,
    TK_ELSE,

    TK_LET,
    TK_VAR,

    TK_PRINT,
    TK_DRAW,

    TK_TEXT,

    TK_T,

    TK_ERROR,
    TK_EOF,
    TK_AS,
    TK_DEF,
    TK_RET,
	TK_REP,
	TK_TO,
    TK_FROM,
	TK_WITH,

    TK_RECT,
    TK_CIRC,
    TK_POLY,
    TK_POLYL,
    TK_PATH,
    TK_ELLIP,
    TK_MOVE,
    TK_TURN,
    TK_VERT,
    TK_JUMP,
    TK_ARC,
    TK_CBEZ,
    TK_QBEZ,
    TK_LINE,

	TK_SIN,
	TK_COS,
	TK_TAN,
	TK_ASIN,
	TK_ACOS,
	TK_ATAN,
	TK_HSIN,
	TK_HCOS,
	TK_RAD,
	TK_SQRT,
} TKType;  

typedef enum{
	CS_ERROR,

	CS_PI,
	CS_TAU,
	CS_E,

	CS_RED,
    CS_ORANGE,
    CS_YELLOW,
    CS_GREEN,
    CS_BLUE,
    CS_PURPLE,
    CS_BROWN,
    CS_MAGENTA,
    CS_OLIVE,
    CS_MAROON,
    CS_NAVY,
    CS_AQUA,
    CS_TURQ,
    CS_SILVER,
    CS_LIME,
    CS_TEAL,
    CS_INDIGO,
    CS_VIOLET,
    CS_PINK,
    CS_BLACK,
    CS_WHITE,
    CS_GRAY,
    CS_GREY,
    CS_TRANSP,
} CSType;

typedef struct{
    TKType type;
    int subtype;
    char* start;
    int length;
    int line;
    int lineIndex;
	int inlineIndex;
} TK; 

void initScanner(char* source);
TK scanTK();

typedef struct {
    char* start;
    char* current;
    char* origin;
    int line;
    char* lastScanned;
	char* lastNewline;
} Scanner;

extern Scanner scanner;
#endif