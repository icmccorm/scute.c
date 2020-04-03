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
	char* lastNewline;
} Scanner;

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

static TKType checkKeyword(int start, int length, char* rest, TKType type){
    if(scanner.current - scanner.start == start + length &&
        memcmp(scanner.start + start, rest, length) == 0){
            return type;
    }
    return TK_ID; 
}

static CSType checkConst(int start, int length, char* rest, CSType type){
	if(scanner.current - scanner.start == start + length &&
        memcmp(scanner.start + start, rest, length) == 0){
            return type;
    }
	return CS_ERROR;
}

static CSType findConstantIdentifier(){
	int length = scanner.current - scanner.start;
	switch(scanner.start[0]){
		case 'p': {
			if(length > 1){
				switch(scanner.start[1]){
					case 'i': {
						if(length > 2){
							switch(scanner.start[2]){
								case 'n': return checkConst(3, 1, "k", CS_PINK);
							}
							return CS_ERROR;
						}
						return CS_PI;
					}
					case 'u': return checkConst(2, 4, "rple", CS_PURPLE);
				}
			}
			return CS_ERROR;
		}	
		case 't': {
			if(length > 1){
				switch(scanner.start[1]){
					case 'a': return checkConst(2, 1, "u", CS_TAU);
					case 'u': return checkConst(2, 7, "rquoise", CS_TURQ);
					case 'e': return checkConst(2, 2, "al", CS_TEAL);
                    case 'r': return checkConst(2, 9, "ansparent", CS_TRANSP);
				}
			}
			return CS_ERROR;			
		}
		case 'e': return CS_E;
		case 'r': return checkConst(1, 2, "ed", CS_RED);
		case 'o': {
			if(length > 1){
				switch(scanner.start[1]){
					case 'r': return checkConst(2, 4, "ange", CS_ORANGE);
					case 'l': return checkConst(2, 3, "ive", CS_OLIVE);
				}
			}
			return CS_ERROR;
		}
		case 'y': return checkConst(1, 5, "ellow", CS_YELLOW);
		case 'b': {
			if(length > 1){
				switch(scanner.start[1]){
					case 'l': {
						if(length > 2){
							switch(scanner.start[2]){
								case 'a': return checkConst(3, 2, "ck", CS_BLACK);
								case 'u': return checkConst(3, 1, "e", CS_BLUE);
							}
						}
					}
					case 'r': return checkConst(2, 3, "own", CS_BROWN);
				}
			}
			return CS_ERROR;
		}
		case 'm': {
			if(length > 1){
				switch(scanner.start[1]){
					case 'a': {
						if(length > 2){
							switch(scanner.start[2]){
								case 'g': return checkConst(3, 4, "enta", CS_MAGENTA);
								case 'r': return checkConst(3, 3, "oon", CS_MAROON);
							}
						}
					}
				}
			}
			return CS_ERROR;
		}
		case 'n': return checkConst(1, 3, "avy", CS_NAVY);
		case 'a': return checkConst(1, 3, "qua", CS_AQUA);
		case 's': return checkConst(1, 5, "ilver", CS_SILVER);
		case 'l': return checkConst(1, 3, "ime", CS_LIME);
		case 'i': return checkConst(1, 5, "ndigo", CS_INDIGO);
		case 'v': return checkConst(1, 5, "iolet", CS_VIOLET);
		case 'w': return checkConst(1, 4, "hite", CS_WHITE);
		case 'g': {
			if(length > 1){
				switch(scanner.start[1]){
					case 'r': {
						if(length > 2){
							switch(scanner.start[2]){
								case 'e': {
									if(length > 3){
										switch(scanner.start[3]){
											case 'e': return checkConst(4, 1, "n", CS_GREEN);
											case 'y': return CS_GREY;
										}
									}
								}
								case 'a': return checkConst(3, 1, "y", CS_GRAY);
							}
						}
					}
				}
			}
			return CS_ERROR;
		}
        default:
            return CS_ERROR;
    }
}

static TKType findIdentifier(){
    switch(scanner.start[0]){
        case 'a':
            if(scanner.current - scanner.start > 1){
                switch(scanner.start[1]){
					case 's': 
						if(scanner.current - scanner.start > 2){
							return checkKeyword(2, 2, "in", TK_HSIN);
						}else{
							return TK_AS;
						}
					case 'c': return checkKeyword(2, 2, "os", TK_HCOS);
                    case 'n': return checkKeyword(2, 1, "d", TK_AND);
                    case 'r': return checkKeyword(2, 1, "c", TK_ARC);
                }
            }
        case 'd':
            if(scanner.current - scanner.start > 1){
                switch(scanner.start[1]){
                    case 'o': return TK_DO;
                    case 'r': return checkKeyword(2, 2, "aw", TK_DRAW);
                    case 'e': 
						if(scanner.current - scanner.start > 2){
							switch(scanner.start[2]){
								case 'f': return TK_DEF;
                                default:
                                    return TK_ID;
							}
						}else{
							return TK_ID;
						}
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
					default:
						return TK_ID;
                }
            }else{
                return TK_ID;
            }
        return checkKeyword(1, 2, "or", TK_FOR);
        case 'i': return checkKeyword(1, 1, "f", TK_IF);
        case 'l': 
            if(scanner.current - scanner.start > 1){
                switch(scanner.start[1]){
                    case 'e': return checkKeyword(2, 1, "t", TK_LET);
                    case 'i': return checkKeyword(2, 2, "ne", TK_LINE);
                }
            }
            return TK_ID;
        case 'w': 
			if(scanner.current - scanner.start > 1){
				switch(scanner.start[1]){
					case 'h': return checkKeyword(2, 3, "ile", TK_WHILE);
					case 'i': return checkKeyword(2, 2, "th", TK_WITH);
				}
			}
			return TK_ID;
        case 'r': 
            if(scanner.current - scanner.start > 1){
                switch(scanner.start[1]){
                    case 'e': {
                        if(scanner.current - scanner.start > 2){
                            switch(scanner.start[2]){
                                case 't': return checkKeyword(3, 3, "urn", TK_RET);
                                case 'c': return checkKeyword(3, 1, "t", TK_RECT);
								case 'p': return checkKeyword(3, 3, "eat", TK_REP);
                            
                            }
                        }
                    } break;
					case 'a': return checkKeyword(2, 5, "dians", TK_RAD);
                }
            }
            return TK_ID;
        case 'c': 
			if(scanner.current - scanner.start > 1){
				switch(scanner.start[1]){
					case 'i': return checkKeyword(2, 4, "rcle", TK_CIRC);
					case 'o': return checkKeyword(2, 1, "s", TK_COS);
                    case 'B': return checkKeyword(2, 5, "ezier", TK_CBEZ);
				}
			}
			return TK_ID;
		case 's': 
			if(scanner.current - scanner.start > 1){
				switch(scanner.start[1]){
					case 'i': return checkKeyword(2, 1, "n", TK_SIN);
					case 'q': return checkKeyword(2, 2, "rt", TK_SQRT);
					default: return TK_ID;
				}
			}
			return TK_ID;
        case 't': 
            if(scanner.current - scanner.start > 1){
                switch(scanner.start[1]){
                    case 'a': return checkKeyword(2, 1, "n", TK_TAN);
                    case 'e': return checkKeyword(2, 2, "xt", TK_TEXT);
                    case 'r': return checkKeyword(2, 2, "ue", TK_TRUE);
					case 'o': {
						if(scanner.current - scanner.start > 2){
							return TK_ID;
						}else{
							return TK_TO;
						}
					} break;
                    case 'u': return checkKeyword(2, 2, "rn", TK_TURN);
                }
            }
            return TK_T;
        case 'o': return checkKeyword(1, 1, "r", TK_OR);
        case 'p': 
            if(scanner.current - scanner.start > 1){
                switch(scanner.start[1]){
                    case 'r': return checkKeyword(2, 3, "int", TK_PRINT);
                    case 'a': return checkKeyword(2, 2, "th", TK_PATH);
                    case 'o': {
                        if(scanner.current - scanner.start > 2){
                            switch(scanner.start[2]){
                                case 'l': {
                                    if(scanner.current - scanner.start > 3){
                                        switch(scanner.start[3]){
                                            case 'y':
                                                if(scanner.current - scanner.start > 4){
                                                    switch(scanner.start[4]){
                                                        case 'l': return checkKeyword(5, 3, "ine", TK_POLYL);
                                                        case 'g': return checkKeyword(5, 2, "on", TK_POLY);
                                                    }
                                                }
                                                break;
                                        }
                                    }
                                } break;
                            }       
                        }
                    } break;
                }
            }
            return TK_ID;
        case 'e':
            if(scanner.current - scanner.start > 1){
                switch(scanner.start[1]){
                    case 'l':
                        if(scanner.current - scanner.start > 2){
                            switch(scanner.start[2]){
                                case 'l': return checkKeyword(3, 4, "ipse", TK_ELLIP);
                                case 's': return checkKeyword(3, 1, "e", TK_ELSE);
                            }
                        }
                    break;
                }
            }
            return TK_ID;
        case 'n': return checkKeyword(1, 3, "ull", TK_NULL);
        case 'v': {
            if(scanner.current - scanner.start > 1){
                switch(scanner.start[1]){
                    case 'e': return checkKeyword(2, 4, "rtex", TK_VERT);
                    case 'a': return checkKeyword(2, 1, "r", TK_VAR);
                }
            }
            return TK_ID;
        } break;
		case 'h':
			if(scanner.current - scanner.start > 1){
				switch(scanner.start[1]){
					case 's': return checkKeyword(2, 2, "in", TK_ASIN);
					case 'c': return checkKeyword(2, 2, "os", TK_ACOS);
					case 't': return checkKeyword(2, 2, "an", TK_ATAN);
					default: return TK_ID;
				}
			}else{
				return TK_ID;
			}
        case 'm': return checkKeyword(1, 3, "ove", TK_MOVE);
        case 'j': return checkKeyword(1, 3, "ump", TK_JUMP);
        case 'q': return checkKeyword(1, 6, "Bezier", TK_QBEZ);
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
        case TK_POLY:
        case TK_POLYL:
        case TK_PATH:
        case TK_MOVE:
        case TK_JUMP:
        case TK_TURN:
        case TK_ARC:
        case TK_VERT:
        case TK_QBEZ:
        case TK_CBEZ:
        case TK_LINE:
            return makeDualToken(TK_SHAPE, type);
        case TK_SIN:
        case TK_COS:
        case TK_TAN:
        case TK_ASIN:
        case TK_ACOS:
        case TK_ATAN:
        case TK_HSIN:
        case TK_HCOS:
        case TK_RAD: 
        case TK_SQRT: 
            return makeDualToken(TK_NATIVE, type);
        default:
            return makeToken(type);
    }
}

static TK constant(){
	while(isAlpha(peek())) advance();
	CSType constType = findConstantIdentifier();
	return makeDualToken(TK_CONST, constType);
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
            if(previous() == '\n' || previous() == '\r'){
                while(*scanner.current == '\t'){
                    advance();
                }
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