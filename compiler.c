#include <stdio.h>
#include "common.h"
#include "compiler.h"
#include "scanner.h"

void compile(const char* source){
    initScanner(source);
    int line = -1;
    for(;;){
        TK token = scanTK();

        if(token.type == TK_EOF) break;

        if(token.line != line){
            printf("%4d ", token.line);
            line = token.line;
        }else{
            printf("   | ");
        }
        switch(token.type){
            case TK_NEWLINE:
                printf("%2d %2d '\\n' \n", token.type, token.indent);
                break;
            default:
                printf("%2d %2d '%.*s' \n", token.type, token.indent, token.length, token.start);
                break;
        }
            
    }
}