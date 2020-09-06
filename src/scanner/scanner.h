#ifndef scute_scanner_h
#define scute_scanner_h
#include "tokenizer.h"

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

CSType checkConstant(int start, int length, char* rest, CSType type);
TKType checkKeyword(int start, int length, char* rest, TKType type);

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