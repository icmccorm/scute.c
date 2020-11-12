#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"
#include "output.h"
#include "tokenizer.h"

Scanner scanner;

void initScanner(char* source){
    scanner.origin = source;
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
    scanner.lastScanned = source;
	scanner.lastNewline = NULL;
}

static TK makeToken(TKType type){
    TK token;
    token.type = type;
    token.line = scanner.line;
    token.start = scanner.start;
    token.length = (int) (scanner.current - scanner.start);
    scanner.lastScanned = scanner.start;
    token.subtype = -1;
    
	if(scanner.lastNewline){
		token.inlineIndex = scanner.start - scanner.lastNewline;
	}else{
		token.inlineIndex = scanner.start - scanner.origin;
	}
	
	return token;
	
}

static TK makeCustomToken(TKType type, char* chars, int length){
	TK custom = makeToken(type);
	custom.start = chars;
	custom.length = length;
	custom.subtype = -1;
    return custom;
}

static TK makeDualToken(TKType primary, int secondary){
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

    if(scanner.start == scanner.current){
	    scanner.lastNewline = scanner.start;
    }else{
        scanner.lastNewline = scanner.current - 1;
    }
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
    return (c >= 'A' && c <= 'Z')
        || (c >= 'a' && c <= 'z')
        || c == '_'; 
}

static TK number(){
    while(isDigit(peek())) advance();

    if(match('.') && isDigit(peek())){
        advance();
        while(isDigit(peek())) advance();

        return makeToken(TK_REAL);
    }
    TK numericalToken = makeToken(TK_INTEGER);
    return numericalToken;
}

TKType checkKeyword(int start, int length, char* rest, TKType type){
    if(scanner.current - scanner.start == start + length 
        && memcmp(scanner.start + start, rest, length) == 0){
            return type;
    }
    return TK_ID;
}

CSType checkConstant(int start, int length, char* rest, CSType type){
    int expectedLength = scanner.current - scanner.start;
	if(expectedLength == start + length &&
        memcmp(scanner.start + start, rest, length) == 0){
            return type;
    }
	return CS_ERROR;
}

static TK identifier(){
    while(isAlpha(peek())) advance();
    TKType type = findKeyword(scanner.start, scanner.current);
    switch(type){
        case TK_RECT:
        case TK_CIRCLE:
        case TK_ELLIPSE:
        case TK_POLYGON:
        case TK_UNGON:
        case TK_POLYLINE:
        case TK_PATH:
        case TK_MOVE:
        case TK_JUMP:
        case TK_TURN:
        case TK_ARC:
        case TK_VERTEX:
        case TK_QBEZIER:
        case TK_CBEZIER:
        case TK_LINE:
        case TK_MIRROR:
            return makeDualToken(TK_SHAPE, type);
        case TK_SIN:
        case TK_COS:
        case TK_TAN:
        case TK_ASIN:
        case TK_ACOS:
        case TK_ATAN:
        case TK_HSIN:
        case TK_HCOS:
        case TK_RADIANS:
        case TK_SQRT:
        case TK_RAND: 
            return makeDualToken(TK_NATIVE, type);
        default:
            return makeToken(type);
    }
}

static TK constant(){
	while(isAlpha(peek())) advance();
	CSType constType = findConstant(scanner.start, scanner.current);
	return makeDualToken(TK_CONSTANT, constType);
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
				++scanner.start;
                return constant();
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
                default:
                    if(isDigit(peek())) {
                        return number();
                    }else{
                        return makeToken(TK_MINUS);
                    }
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
            if(previous() == '\n' || previous() == '\r'){
                while(*scanner.current == '\t'){
                    advance();
                }
                if(*scanner.current == '\n'){
                    advance();
                    return scanTK();
                }else{
                    return makeToken(TK_INDENT);
                }
            }else{
                return scanTK();
            }
            break;
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