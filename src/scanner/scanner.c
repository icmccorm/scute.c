#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"
#include "output.h"

typedef struct {
    char* start;
    char* current;
    char* origin;
    int line;
    char* lastScanned;
} Scanner;

Scanner scanner;

void initScanner(char* source){
    scanner.origin = source;
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
    scanner.lastScanned = source;
}

static TK makeToken(TKType type){
    TK token;
    token.type = type;
    token.line = scanner.line;
    token.start = scanner.start;
    token.length = (int) (scanner.current - scanner.start);
    scanner.lastScanned = scanner.start;
    token.subtype = -1;
    return token;
}

static TK makeCustomToken(TKType type, char* chars, int length){
    TK token;
    token.type = type;
    token.line = scanner.line;
    token.start = chars;
    token.length = length;
    scanner.lastScanned = scanner.start;
    token.subtype = -1;
    return token;
}

static TK makeDualToken(TKType primary, TKType secondary){
    TK token = makeToken(primary);
    token.subtype = secondary;
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

static bool match(char c){
    if(isAtEnd()) return false;
    if(*scanner.current != c) return false;

    ++scanner.current;
    return true;
}

static TK errorToken(char* s){
    TK token;
    token.type = TK_ERROR;
    token.start = s;
    token.line = scanner.line;
    token.length = (int) strlen(s);
    return token;
}

static char peek(){
    if(isAtEnd()) return '\0';
    return *scanner.current;
}

static TK makeNewline(){
    ++scanner.line;

    TK token;
    token.type = TK_NEWLINE;
    token.line = scanner.line;
    token.start = scanner.start;

    while(peek() == '\n'){
        advance();
    }
    token.length = (int) (scanner.current - scanner.start);
    scanner.lastScanned = scanner.start;
    return token;
}

static void skipWhiteSpace(){
    for(;;){
        switch(peek()){
            case ' ':
                advance();
            default:
                return;
        }
    }
}

static char previous(){
    return *scanner.lastScanned;
}

static bool isDigit(char c){
    return c >= '0' && c <= '9';
}

static bool isAlpha(char c){
    return c >= 'A' && c <= 'Z'
        || c >= 'a' && c <= 'z'
        || c == '_'; 
}

static TK number(){
    while(isDigit(peek())) advance();

    if(match('.') && isDigit(peek())){
        advance();
        while(isDigit(peek())) advance();

        return makeToken(TK_REAL);
    }

    return makeToken(TK_INTEGER);
}

static TKType checkKeyword(int start, int length, char* rest, TKType type){
    if(scanner.current - scanner.start == start + length &&
        memcmp(scanner.start + start, rest, length) == 0){
            return type;
    }
    return TK_ID; 
}

static TKType findIdentifier(){
    switch(scanner.start[0]){
        case 'a':
            if(scanner.current - scanner.start > 1){
                switch(scanner.start[1]){
                    case 'n': return checkKeyword(2, 1, "d", TK_AND);
                    case 's': return TK_AS;
                }
            }
        case 'd':
            if(scanner.current - scanner.start > 1){
                switch(scanner.start[1]){
                    case 'o': return TK_DO;
                    case 'r': return checkKeyword(2, 2, "aw", TK_DRAW);
                    case 'i': return checkKeyword(2, 2, "ms", TK_DIMS);
                    case 'e': return checkKeyword(2, 1, "f", TK_DEF);
                }
            }else{
                return TK_ID;
            }
        case 'f': 
            if(scanner.current - scanner.start > 1){
                switch(scanner.start[1]){
                    case 'o': return checkKeyword(2, 1, "r", TK_FOR);
                    case 'a': return checkKeyword(2, 3, "lse", TK_FALSE);
                    case 'u': return checkKeyword(2, 2, "nc", TK_FUNC);
                    case 'r': return checkKeyword(2, 2, "om", TK_FROM);
                }
            }else{
                return TK_ID;
            }
        return checkKeyword(1, 2, "or", TK_FOR);
        case 'i': return checkKeyword(1, 1, "f", TK_IF);
        case 'l': return checkKeyword(1, 2, "et", TK_LET);
        case 'w': return checkKeyword(1, 4, "hile", TK_WHILE);
        case 'r': 
            if(scanner.current - scanner.start > 1){
                switch(scanner.start[1]){
                    case 'e': {
                        if(scanner.current - scanner.start > 2){
                            switch(scanner.start[2]){
                                case 't': return checkKeyword(3, 3, "urn", TK_RET);
                                case 'c': return checkKeyword(3, 1, "t", TK_RECT);
                                default:
                                    return TK_ID;
                            }
                        }else{
                            return TK_ID;
                        }
                    }
                    default:
                        return TK_ID;
                }
            }else{
                return TK_ID;
            }
        case 'c': return checkKeyword(1, 5, "ircle", TK_CIRC);
        case 't': 
            if(scanner.current - scanner.start > 1){
                switch(scanner.start[1]){
                    case 'a': return checkKeyword(2, 1, "u", TK_TAU);
                    case 'e': return checkKeyword(2, 2, "xt", TK_TEXT);
                    case 'r': return checkKeyword(2, 2, "ue", TK_TRUE);
                }
            }else{
                return TK_T;
            }
        case 'o': return checkKeyword(1, 1, "r", TK_OR);
        case 'p': 
            if(scanner.current - scanner.start > 1){
                switch(scanner.start[1]){
                    case 'r': return checkKeyword(2, 3, "int", TK_PRINT);
                    case 'o': {
                        if(scanner.current - scanner.start > 2){
                            switch(scanner.start[2]){
                                case 'l': return checkKeyword(3, 1, "y", TK_POLY);
                                case 's': return TK_POS;
                            }    
                        }else{
                            return TK_ID;
                        }
                        
                        } break;
                    case 'i': return TK_PI;
                }
            }else{
                return TK_ID;
            }
        return checkKeyword(1, 1, "i", TK_PI);
        case 'e':
            if(scanner.current - scanner.start > 1){
                switch(scanner.start[1]){
                    case 'l':
                        if(scanner.current - scanner.start > 2){
                            switch(scanner.start[2]){
                                case 'l': return checkKeyword(3, 4, "ipse", TK_ELLIP);
                                case 's': return checkKeyword(3, 1, "e", TK_ELSE);
                            }
                        }else{
                            return TK_ID;
                        }
                }
            }else{
                return TK_E;
            }
        case 'n': return checkKeyword(1, 3, "ull", TK_NULL);
        case 'v': return checkKeyword(1, 2, "ar", TK_VAR);
        default:
            return TK_ID;
    }
}

static TK identifier(){
    while(isAlpha(peek())) advance();
    TKType type = findIdentifier();
    switch(type){
        case TK_RECT:
        case TK_CIRC:
        case TK_ELLIP:
            return makeDualToken(TK_SHAPE, type);
/*      case TK_RED:
        case TK_ORANGE:
        case TK_YELLOW:
        case TK_GREEN:
        case TK_BLUE:
        case TK_PURPLE:
        case TK_BROWN:
        case TK_MAGENTA:
        case TK_TAN:
        case TK_OLIVE:
        case TK_MAROON:
        case TK_NAVY:
        case TK_AQUM:
        case TK_TURQ:
        case TK_SILVER:
        case TK_LIME:
        case TK_TEAL:
        case TK_INDIGO:
        case TK_VIOLET:
        case TK_PINK:
        case TK_BLACK:
        case TK_WHITE:
        case TK_GRAY:
        case TK_GREY:
            return makeDualToken(TK_CONST, type);*/
        default:
            return makeToken(type);
    }
}

static TK constant(){
    while(isAlpha(peek())) advance();
    TKType type = findIdentifier();
    switch(type){
        
    }
}

static TK string(char c){
    ++scanner.start;
    while(!isAtEnd() && peek() != c && peek() != '\n'){
        match('\\');
        advance();
    }
    if(isAtEnd() || peek() == '\n') return errorToken("Unterminated string.");
    TK token = makeToken(TK_STRING);
    advance();
    return token;
}

TK scanTK(){
    skipWhiteSpace();

    if(isAtEnd()) return makeToken(TK_EOF);

    scanner.start = scanner.current;

    char c = advance();
    switch(c){
        case ' ': {
            int spaceCount = 1;
            while(peek() == ' ' && spaceCount < 4){
                ++spaceCount;
                advance();
            }
            if(spaceCount == 4){
                return makeToken(TK_INDENT);
            }else{
                return scanTK();
            }
        } break;
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
        case ';': return makeToken(TK_SEMI);

        case ':':{
            char b = advance();
            if(isAlpha(b)){
                return identifier();
            }else{
                if(match('=')){
                    return makeToken(TK_EVAL_ASSIGN);
                }else{
                    return makeToken(TK_COLON); 
                }
            }
        } break;
        case '<': 
            if(match('=')){
                return makeToken(TK_LESS_EQUALS);
            }else if(match('-') && peek() != '-'){
                return makeToken(TK_L_LIMIT);
            }else{
                return makeToken(TK_LESS);
            }
        case '>': return makeToken(match('=') ? TK_GREATER_EQUALS : TK_GREATER);
        case '=': return makeToken(match('=') ? TK_EQUALS : TK_ASSIGN);
        case '!': return makeToken(match('=') ? TK_BANG_EQUALS : TK_BANG);

        case '/':
            if(match('/')){
                while(peek() != '\n' && !isAtEnd()){
                    advance();
                }
                return scanTK();
            }else if(match('*')){
                while(!(peek() == '*' && peekNext() == '/') && !isAtEnd()){
                    advance();
                }
                advance(); // clear '*'
                advance(); // clear '/'
                return scanTK();
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
                case '>':
                    ++scanner.current;
                    return makeToken(TK_R_LIMIT);
                default:
                    return makeToken(TK_MINUS);
            }
        case '\'':
        case '"':
                return string(c);
            break;
        case '\r':
            if(match('\n')){
                return makeNewline();
            }else{
                return scanTK();
            }
        case '\n':
            return makeNewline();
        case '\t':
            /* \r is included to handle Windows' \r\n. */
            if(previous() == '\n' || previous() == '\t' || previous() == '\r'){
                return makeToken(TK_INDENT);

            }else{
                return scanTK();
            }
        default:
            if(isDigit(c)) return number();
            if(isAlpha(c)) return identifier();
            return errorToken("Unrecognized character.");
            break;
    }
}

static void printToken(TK token){
    print(O_DEBUG, "%4d ", token.line);
    switch(token.type){
        case TK_INDENT:
            print(O_DEBUG, "%2d '\\t' \n", token.type);
            break;
        case TK_NEWLINE:
            print(O_DEBUG, "%2d '\\n' \n", token.type);
            break;
        default:
            print(O_DEBUG, "%2d '%.*s' \n", token.type, token.length, token.start);
            break;
    }
}