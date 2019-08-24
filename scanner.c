#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

typedef struct {
    const char* start;
    const char* current;
    const char* origin;
    int line;
    int indent;
} Scanner;

Scanner scanner;



static TK makeToken(TKType type){
    TK token;
    token.type = type;
    token.line = scanner.line;
    token.start = scanner.start;
    token.length = (int) (scanner.current - scanner.start);
    token.indent = scanner.indent;
    return token;
}

static bool isAtEnd(){
    return *scanner.current == '\0';
}

static char advance(){
    ++scanner.current;
    return scanner.current[-1];
}

static char peekNext(){
    if(isAtEnd()) return '\0';
    return scanner.current[1];
}

static bool match(const char c){
    if(isAtEnd()) return false;
    if(*scanner.current != c) return false;

    ++scanner.current;
    return true;
}

static TK errorToken(const char* s){
    TK token;
    token.type = TK_ERROR;
    token.start = s;
    token.line = scanner.line;
    token.length = (int) strlen(s);
    token.indent = scanner.indent;
    return token;
}

static char peek(){
    if(isAtEnd()) return '\0';
    return *scanner.current;
}

static TK makeNewline(){
    scanner.indent = 0;
    ++scanner.line;

    TK token;
    token.type = TK_NEWLINE;
    token.line = scanner.line;
    token.indent = scanner.indent;
    token.start = scanner.start;

    while(peek() == '\n'){
        advance();
    }

    token.length = (int) (scanner.current - scanner.start);

    while(peek() == '\t' || peek() == ' '){
        if(peek() == '\t') ++scanner.indent;
        advance();
    }

    return token;
}

TK scanTK(){

    scanner.start = scanner.current;
    if(isAtEnd()) return makeToken(TK_EOF);

    const char c = advance();

    switch(c){
        case '*': return makeToken(TK_TIMES);
        case '(': return makeToken(TK_L_PAREN);
        case ')': return makeToken(TK_R_PAREN);
        case '[': return makeToken(TK_L_BRACK);
        case ']': return makeToken(TK_R_BRACK);
        case ',': return makeToken(TK_COMMA);
        case '%': return makeToken(TK_MODULO);
        case '.': return makeToken(TK_DEREF);
        case '~': return makeToken(TK_TILDA);
        case '?': return makeToken(TK_QUESTION);

        case ':': return makeToken(match('=') ? TK_EVAL_ASSIGN : TK_COLON);
        case '<': return makeToken(match('=') ? TK_LESS_EQUALS : TK_LESS);
        case '>': return makeToken(match('=') ? TK_GREATER_EQUALS : TK_GREATER);
        case '=': return makeToken(match('=') ? TK_EQUALS : TK_ASSIGN);
        case '!': return makeToken(match('=') ? TK_BANG_EQUALS : TK_BANG);

        case '/':
            if(match('/')){
                while(peek() != '\n' && !isAtEnd()){
                    advance();
                }
            }else if(match('*')){
                while((peek() != '*' && peekNext() != '/') && !isAtEnd()){
                    advance();
                }
            }else{
                return makeToken(TK_DIVIDE);
            }

        case '+':
            switch(peek()){
                case '=': 
                    ++scanner.current;
                    return makeToken(TK_INCR_ASSIGN);
                
                case '+': 
                    ++scanner.current;
                    return makeToken(TK_INCR);

                default:
                    return makeToken(TK_PLUS);
            }

        case '-':
            switch(peek()){
                case '=': 
                    ++scanner.current;
                    return makeToken(TK_DECR_ASSIGN);
                
                case '-': 
                    ++scanner.current;
                    return makeToken(TK_DECR);

                default:
                    return makeToken(TK_MINUS);
            }
        case '\'':
        case '"':
            break;
        case '\r':
            if(match('\n')){
                return makeNewline();
            }else{
                return scanTK();
            }
        case '\t':
        case ' ':
            return scanTK();
        default:
            //handle integers and IDs.
            break;
    }
}

void initScanner(const char* source){
    scanner.origin = source;
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
    scanner.indent = 0;

    while(peek() == '\t' || peek() == ' '){
        if(peek() == '\t') ++scanner.indent;
        advance();
    }

}