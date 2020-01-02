#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "chunk.h"
#include "scanner.h"
#include "value.h"
#include "obj.h"
#include "vm.h"
#include "output.h"
#include "compiler_defs.h"
#include "debug.h"
#include "vm.h"
#include "package.h"

Parser parser;
Compiler* current = NULL;
Chunk* compilingChunk;
CompilePackage* result;

int getTokenIndex(char* tokenStart){
	return (unsigned) (tokenStart - parser.codeStart);
}

void initCompiler(Compiler* compiler, CompilePackage* package){
	compiler->scopeDepth = 0;
	compiler->scopeCapacity = 0;
	compiler->localCount = 0;
	compiler->locals = NULL;
	compiler->enclosed = false;
	current = compiler;
	compiler->shapeType = 0;
}

static Chunk* currentChunk() {
	return compilingChunk;
}

static Compiler* currentCompiler() {
	return current;
}

CompilePackage* currentResult(){
	return result;
}

static void beginScope(){
	++currentCompiler()->scopeDepth;
}

static void endScope(){
	--currentCompiler()->scopeDepth;
}

static void emitByte(uint8_t byte){
	writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2){
	emitByte(byte1);
	emitByte(byte2);
}

static void emitReturn(){
	emitByte(OP_RETURN);
}

static void emitBundle(uint8_t opCode, Value index){
	writeOperatorBundle(currentChunk(), opCode, AS_NUM(index), parser.previous.line);
}

static void emitTriple(uint8_t opCode, Value val1, Value val2){
	writeChunk(currentChunk(), opCode, parser.previous.line);
	writeVariableData(currentChunk(), AS_NUM(val1));
	writeVariableData(currentChunk(), AS_NUM(val2));

}

static int emitLimit(int low, int high){
	emitByte(OP_LIMIT);
	int offset = 0;
	offset += writeVariableData(currentChunk(), low);
	offset += writeVariableData(currentChunk(), high);
	emitByte(0xFF);
	emitByte(0xFF);
	return currentChunk()->count - 2;
}

static void emitConstant(Value value){
	writeConstant(currentChunk(), value, parser.previous.line);
}

static void errorAt(TK* token, char* message){
	if(parser.panicMode) return;
	parser.panicMode = true;
	print(O_ERR, "[line %d] Error", token->line);

	if(token->type == TK_EOF){
		print(O_ERR, " at end");
	} else if (token->type == TK_ERROR){
		print(O_ERR, "[line %d] %.*s", token->line, token->length, token->start);
	} else {
		print(O_ERR, " at '%.*s'", token->length, token->start);
	}
	print(O_ERR, ": %s\n", message);
	parser.hadError = true;
}

static void errorAtCurrent(char* message){
	errorAt(&parser.current, message);
}

static void error(char* message){
	errorAt(&parser.previous, message);
}

static TKType advance() {
	if(parser.current.type == TK_DEREF && parser.previous.type == TK_ID) parser.lastID = parser.previous;
	parser.previous = parser.current;
	
	for(;;){
		parser.current = scanTK();
		if(parser.current.type != TK_ERROR) {
			return parser.current.type;
		}
		errorAtCurrent(parser.current.start);
	}
}

static void consume(TKType type, char* message){
	if(parser.current.type == type){
		advance();
		return;
	}
	errorAtCurrent(message);
}

static bool check(TKType type){
	return parser.current.type == type;
}

static bool match(TKType type){
	if(!check(type)) return false;
	advance();
	return true;
}

static bool tokensEqual(TK one, TK two){
	if(one.length != two.length) return false;
	return memcmp(one.start, two.start, one.length) == 0;
}

static void endLine(){
	if(parser.current.type != TK_EOF) consume(TK_NEWLINE, "Expected end of line, '\\n'");
}

static void endCompiler(){
	emitReturn();

	#ifdef DEBUG_PRINT_CODE
	#include "debug.h"
	if(!parser.hadError){
		printChunk(currentChunk(), "code");
	}
	#endif
}

static Value getTokenStringObj(TK* token){
	Value strObj = OBJ_VAL(internString(token->start, token->length));
	strObj.charIndex = getTokenIndex(token->start);
	strObj.line = token->line;
	return strObj;
}

static uint32_t getStringObjectIndex(TK* token){
	if(token == NULL) return -1;
	return writeValue(currentChunk(), getTokenStringObj(token), parser.previous.line);
}

static uint32_t getObjectIndex(Obj* obj){
	if(obj == NULL) return -1;
	return writeValue(currentChunk(), OBJ_VAL(obj), parser.previous.line);
}

static uint32_t getChunkIndex(Chunk* chunk){
	if(chunk == NULL) return -1;
	return writeValue(currentChunk(), CHUNK_VAL(chunk), parser.previous.line);
}

static void expression();
static void block();

static void and_(bool canAssign);
static void binary(bool canAssign);
static void unary(bool canAssign);
static void grouping(bool canAssign);
static void number(bool canAssign);
static void literal(bool canAssign);
static void constant(bool canAssign);
static void string(bool canAssign);
static void variable(bool canAssign);
static void deref(bool canAssign);
static void scopeDeref(bool canAssign);

ParseRule rules[] = {
	{ NULL,     binary,     PC_TERM },    // TK_PLUS,
	{ unary,    binary,     PC_TERM },    // TK_MINUS,
	{ NULL,     binary,     PC_FACTOR },  // TK_TIMES,
	{ NULL,     binary,     PC_FACTOR },  // TK_DIVIDE,
	{ NULL,     binary,     PC_FACTOR },  // TK_MODULO,
	{ NULL,     binary,     PC_EQUALS },  // TK_EQUALS,
	{ NULL,	    binary,	    PC_EQUALS },  // TK_BANG_EQUALS,
	{ NULL,	    binary,	    PC_COMPARE }, // TK_LESS_EQUALS,
	{ NULL,	    binary,	    PC_COMPARE }, // TK_GREATER_EQUALS,
	{ NULL,	    binary,	    PC_COMPARE }, // TK_LESS,
	{ NULL,	    binary,	    PC_COMPARE }, // TK_GREATER,
	{ NULL,     binary,     PC_ASSIGN },  // TK_ASSIGN,
	{ NULL,     binary,     PC_ASSIGN },  // TK_INCR_ASSIGN,
	{ NULL,     binary,     PC_ASSIGN },  // TK_DECR_ASSIGN,
	{ unary,    NULL,       PC_UNARY },   // TK_BANG,
	{ NULL,	    NULL,	    PC_UNARY },   // TK_INCR, 
	{ NULL,	    NULL,	    PC_UNARY },   // TK_DECR,
	{ NULL,	    NULL,	    PC_NONE },    // TK_COLON,
	{ NULL,	    NULL,	    PC_NONE },    // TK_QUESTION,
	{ NULL,	    NULL,	    PC_NONE },    // TK_EVAL_ASSIGN,
	{ NULL,	    NULL,	    PC_NONE },    // TK_L_LIMIT, 
	{ NULL,	    NULL,	    PC_NONE },    // TK_R_LIMIT,
	{ literal,  NULL,       PC_NONE },    // TK_REAL,
	{ literal,  NULL,       PC_NONE },    // TK_INTEGER,
	{ literal,	NULL,	    PC_NONE },    // TK_TRUE,
	{ literal,	NULL,	    PC_NONE },    // TK_FALSE,
	{ literal,	NULL,	    PC_NONE },    // TK_NULL,
	{ string,	NULL,	    PC_NONE },    // TK_STRING,
	{ variable, NULL,	    PC_NONE },    // TK_ID,
	{ NULL,	    NULL,	    PC_NONE },    // TK_FUNC,
	{ NULL,	    and_,	    PC_AND },     // TK_AND,
	{ NULL,	    NULL,	    PC_NONE },    // TK_OR,
	{ NULL,	    NULL,	    PC_NONE },    // TK_PRE,
	{ literal,	NULL,	    PC_NONE },    // TK_PI,
	{ literal,	NULL,	    PC_NONE },    // TK_E,
	{ literal,	NULL,	    PC_NONE },    // TK_TAU,
	{ NULL,	    NULL,	    PC_NONE },    // TK_SEMI,
	{ NULL,	    NULL,	    PC_NONE },    // TK_L_BRACE,
	{ NULL,	    NULL,	    PC_NONE },    // TK_R_BRACE,
	{ grouping, NULL,       PC_NONE },    // TK_L_PAREN,
	{ NULL,	    NULL,	    PC_NONE },    // TK_R_PAREN, 
	{ NULL,	    NULL,	    PC_NONE },    // TK_L_BRACK,
	{ NULL,	    NULL,	    PC_NONE },    // TK_R_BRACK,
	{ NULL,	    NULL,	    PC_NONE },    // TK_COMMA,
	{ scopeDeref,	deref,	PC_CALL },    // TK_DEREF, 
	{ NULL,	    NULL,	    PC_NONE },    // TK_TILDA, 
	{ NULL,	    NULL,	    PC_NONE },    // TK_NEWLINE,
	{ NULL,	    NULL,	    PC_NONE },    // TK_INDENT,
	{ NULL,	    NULL,	    PC_NONE },    // TK_DO,
	{ NULL,	    NULL,	    PC_NONE },    // TK_WHILE,
	{ NULL,	    NULL,	    PC_NONE },    // TK_FOR,
	{ NULL,	    NULL,	    PC_NONE },    // TK_IF,
	{ NULL,	    NULL,	    PC_NONE },    // TK_ELSE,
	{ NULL,	    NULL,	    PC_NONE },    // TK_RECT,
	{ NULL,	    NULL,	    PC_NONE },    // TK_CIRC,
	{ NULL,	    NULL,	    PC_NONE },    // TK_ELLIP,
	{ NULL,	    NULL,	    PC_NONE },    // TK_LET,
	{ NULL,	    NULL,	    PC_NONE },    // TK_VAR,
	{ NULL,	    NULL,	    PC_PRIMARY }, // TK_PRINT,
	{ NULL,	    NULL,	    PC_NONE },    // TK_DRAW,
	{ NULL,	    NULL,	    PC_NONE },    // TK_TEXT,
	{ NULL,		NULL,		PC_NONE },    // TK_T,
	{ NULL, 	NULL, 		PC_NONE },	  // TK_DIMS
	{ NULL,		NULL,		PC_NONE }, 	  // TK_POS
	{ NULL,	    NULL,	    PC_NONE },    // TK_ERROR,
	{ NULL,     NULL,       PC_NONE }     // TK_EOF
};

static void printStatement();
static void expressionStatement();
static void assignStatement(bool enforceGlobal);
static void defStatement();
static void frameStatement();
static void synchronize();
static uint8_t emitParams();
static void attr();
static void drawStatement();

static ParseRule* getRule(TKType type){
	return &rules[type];
}

static void parsePrecedence(PCType precedence){
	advance();
	ParseFn prefixRule = getRule(parser.previous.type)->prefix;
	if(prefixRule == NULL){
		errorAtCurrent("Expect expression.");
		return;
	}
	bool canAssign = precedence <= PC_ASSIGN;
	prefixRule(canAssign);
	while(precedence <= getRule(parser.current.type)->precedence){
		advance();
		ParseFn infix = getRule(parser.previous.type)->infix;
		infix(canAssign);
	}
	if(!canAssign && match(TK_ASSIGN)){
		error("Invalid assignment target.");
		expression();
	}
}
static void statement(){
	while(parser.current.type == TK_NEWLINE || parser.current.type == TK_INDENT){
		advance();
	}
	switch(parser.current.type){
		case TK_DEF:
			advance();
			defStatement();
			break;
		case TK_DRAW:
			advance();
			drawStatement();
			break;
		case TK_POS:
		case TK_DIMS:
			advance();
			attr();
			break;
		case TK_PRINT:
			advance();
			printStatement();
			break;
		case TK_LET:
			advance();
			assignStatement(false);
			break;
		case TK_VAR:
			advance();
			assignStatement(true);
			break;
		case TK_INTEGER:
		case TK_T:
			advance();
			frameStatement();
			break;
		default:
			expressionStatement();
			break;
	}
	if(parser.panicMode){
		synchronize();
	}
}

static void synchronize(){
	parser.panicMode = false;
	while(parser.current.type != TK_EOF){
		if(parser.previous.type == TK_NEWLINE) return;
		switch(parser.current.type){
			case TK_PRINT:
			case TK_DRAW:
			case TK_LET:
				return;
			default: ;
		}
		advance();
	}
}

static void expression() {
	parsePrecedence(PC_ASSIGN);
}
static void number(bool canAssign) {
	double value = strtod(parser.previous.start, NULL);
	Value val = NUM_VAL(value);
	val.charIndex = getTokenIndex(parser.previous.start);
	val.line = parser.previous.line;
	emitConstant(val);
}

static void literal(bool canAssign) {
	switch(parser.previous.type){
		case TK_FALSE:  emitByte(OP_FALSE); break;
		case TK_TRUE:   emitByte(OP_TRUE); break;
		case TK_NULL:   emitByte(OP_NULL); break;
		case TK_TAU:    emitByte(OP_TAU); break;
		case TK_PI:     emitByte(OP_PI); break;
		case TK_E:      emitByte(OP_E); break;
		case TK_INTEGER:
		case TK_REAL:
			number(canAssign);
			break;
		default:
			return;
	}    
}

static void attr(){
	TK attrType = parser.previous;
	uint8_t attrOp;

	switch(attrType.type){
		case TK_DIMS:
			attrOp = OP_DIMS;
			break;
		case TK_POS:
			attrOp = OP_POS;
			break;
		default:
			break;
	}
	uint8_t paramCount = emitParams();
	emitBytes(attrOp, paramCount);
	endLine();
}



static int getIndentation (){
	int indentCount = 0;
	 while(parser.current.type == TK_INDENT || parser.current.type == TK_NEWLINE){
		if(parser.current.type == TK_INDENT){
			++indentCount;
		}else{
			indentCount = 0;
		}
		advance();
	}
	return indentCount;
}

static void block(){
	beginScope();
	while(parser.current.type != TK_EOF 
			&& getIndentation() >= currentCompiler()->scopeDepth
		){

		statement();
	}
	endScope();

	Compiler* currentComp = currentCompiler();
	while(currentComp->localCount > 0
			&& currentComp->locals[currentComp->localCount-1].depth 
				> currentComp->scopeDepth){
		emitByte(OP_POP);
		--currentComp->localCount;
	}
}

static uint32_t chunkBlock(){
	Chunk* chunkObj = ALLOCATE(Chunk, 1);
	initChunk(chunkObj);

	Chunk* originalChunk = compilingChunk;
	compilingChunk = chunkObj;
	
	block();

	compilingChunk = originalChunk;
	return getChunkIndex(chunkObj);
}

static int emitJump(OpCode op){
	emitByte(op);
	emitByte(0xFF);
	emitByte(0xFF);
	return currentChunk()->count - 2;
}

static void patchJump(int jumpIndex){
	uint16_t backIndex = currentChunk()->count - jumpIndex - 2;

	currentChunk()->code[jumpIndex] = (backIndex >> 8) & 0xFF;
	currentChunk()->code[jumpIndex + 1] = (backIndex) & 0xFF;
}

static void emitScope(TK* idToken, TK* superToken, TKType type, uint32_t chunkIndex){
	emitByte(OP_SCOPE);

	uint32_t idIndex = getStringObjectIndex(idToken);
	writeVariableData(currentChunk(), idIndex);
	if(superToken != NULL){
		uint32_t idIndex = getStringObjectIndex(superToken);
		writeVariableData(currentChunk(), idIndex);
	}else{
		emitByte(0);
	}
	emitByte(type);
	writeVariableData(currentChunk(), chunkIndex);
}

static void emitShapeScope(TK* idToken, TKType shapeType){
	emitByte(OP_SCOPE);

	uint32_t idIndex = getStringObjectIndex(idToken);
	writeVariableData(currentChunk(), idIndex);
	emitByte(0);
	emitByte(0);	
}

static void drawStatement(){
	Compiler* currentComp = currentCompiler();
	if(parser.current.type == TK_ID){
		advance();
		TK idToken = parser.previous;
		advance();
		if(parser.previous.type == TK_AS){ //draw foo as ____
			advance();
			TK shapeToken = parser.previous;

			switch(shapeToken.type){
				case TK_SHAPE:
					endLine();
					currentCompiler()->enclosed = true;
					uint32_t chunkIndex = chunkBlock();
					currentCompiler()->enclosed = false;

					emitScope(&idToken, NULL, shapeToken.subtype, chunkIndex);
					break;
				default:
					errorAt(&parser.previous, "Expected a shape identifier.");
			}
		}else{	// draw foo
			endLine();
			//FIX: resolve variable name (foo) to scope
			
		}
	}else if(parser.current.type == TK_SHAPE){	//draw ___ <= rect, circle, etc.
		advance();
		TK shapeToken = parser.previous;
		endLine();
		
		currentCompiler()->enclosed = true;
		uint32_t chunkIndex = chunkBlock();
		currentCompiler()->enclosed = false;

		emitScope(NULL, NULL, shapeToken.subtype, chunkIndex);
	}

	emitByte(OP_DRAW);
}

static void defStatement(){
	Compiler* currentComp = currentCompiler();
	consume(TK_ID, "Expected an identifier.");

	TK idToken = parser.previous;
	advance();

	if(parser.previous.type == TK_AS){
		advance();
		TK shapeToken = parser.previous;
		switch(shapeToken.type){
			case TK_SHAPE:
				endLine();
				currentCompiler()->enclosed = true;
				uint32_t chunkIndex = chunkBlock();
				currentCompiler()->enclosed = false;		
				emitScope(&idToken, NULL, shapeToken.subtype, chunkIndex);
				break;
			default:
				errorAt(&parser.current, "Expected a shape identifier.");
				break;
		}
	}else{
		endLine();
		currentCompiler()->enclosed = true;
		uint32_t chunkIndex = chunkBlock();
		currentCompiler()->enclosed = false;
		emitScope(&idToken, NULL, 0, chunkIndex);
	}
}

static void frameStatement() {
	if(parser.previous.type == TK_T){
		consume(TK_R_LIMIT, "Expected right limit.");
	}
	consume(TK_INTEGER, "Expected upper-bound");
	TK upper = parser.previous;
	int upperVal = (strtod(parser.previous.start, NULL));	
	
	if((upperVal) <= 0){
		error("Invalid upper bound.");
	}
	if((upperVal) > currentResult()->upperLimit) currentResult()->upperLimit = upperVal;

	int jumpIndex = emitLimit(currentResult()->lowerLimit, upperVal);
	endLine();
	block();
	patchJump(jumpIndex);
}

static void and_(bool canAssign){
	int endJump = emitJump(OP_JMP_FALSE);
	emitByte(OP_POP);
	parsePrecedence(PC_AND);

	patchJump(endJump);
}

static void string(bool canAssign){
	emitConstant(getTokenStringObj(&parser.previous));
}

static void timeStep(bool canAssign){
	emitByte(OP_T);
}

static void ifStatement(){
}

static uint8_t emitParams(){
	uint8_t paramCount = 0;
	consume(TK_L_PAREN, "Expected '('.");
	expression();
	if(!parser.panicMode) ++paramCount;
	while(parser.current.type == TK_COMMA){
		advance();
		expression();
		if(!parser.panicMode) ++paramCount;
	}
	consume(TK_R_PAREN, "Expected ')'.");
	return paramCount;
}

static void printStatement(){
	expression();
	emitByte(OP_PRINT);
	endLine();
}

static void dimsStatement(){
	uint8_t numParams = emitParams();
	emitBytes(OP_DIMS, numParams);
	endLine();
}

static void posStatement(){
	uint8_t numParams = emitParams();
	emitBytes(OP_POS, numParams);
	endLine();
}

static void expressionStatement(){
	expression();
	//emitByte(OP_POP);
	endLine();
}

static void parseAssignment(){
	if(match(TK_ASSIGN)){
		expression();
	}else{
		emitByte(OP_NULL);
	}
}

static void scopeDeref(bool canAssign){
	if(currentCompiler()->enclosed){
		emitByte(OP_LOAD_SCOPE);
		deref(canAssign);
		
	}else{
		errorAtCurrent("Cannot set a scope property outside of a scope");
	}

}

static void deref(bool canAssign){
	while(parser.previous.type == TK_DEREF){
		TK possibleID = parser.current;
		if(possibleID.type == TK_ID){
			advance();
			if(parser.current.type == TK_DEREF){
				uint32_t idIndex = getStringObjectIndex(&parser.previous);
				
				Value indexVal = NUM_VAL(idIndex);
				indexVal.charIndex = getTokenIndex(parser.previous.start);
				indexVal.line = parser.previous.line;

				emitBundle(OP_GET_SCOPE, indexVal);
				advance();
			}else{
				//reached the end of the deref chain; determing whether an assignment is needed.
				uint32_t idIndex = getStringObjectIndex(&parser.previous);
				uint32_t encloseIndex = getStringObjectIndex(&parser.lastID);
				Value indexVal = NUM_VAL(idIndex);
				Value encloseVal = NUM_VAL(encloseIndex);
				indexVal.charIndex = getTokenIndex(parser.previous.start);
				indexVal.line = parser.previous.line;
				
				if(canAssign && match(TK_ASSIGN)){
					expression();
					emitTriple(OP_DEF_SCOPE, indexVal, encloseVal);
				}else{
					emitBundle(OP_GET_SCOPE, indexVal);
				}
			}
		}else{
			errorAtCurrent("Expected an identifier.");
		}
	}
}

static int32_t resolveLocal(TK*id){
	Compiler* comp = currentCompiler();
	for(int i = comp->localCount-1; i>=0; --i){
		Local* currentLocal = &comp->locals[i];
		if(tokensEqual(*id, currentLocal->id)){
			return i;
		}
	}
	return (int32_t) -1;
}

static void namedLocal(TK* id, bool canAssign, uint32_t index){
	Value indexVal = NUM_VAL(index);
	indexVal.charIndex = getTokenIndex(id->start);
	indexVal.line = id->line;
	if(canAssign && match(TK_ASSIGN)){
		expression();
		emitBundle(OP_DEF_LOCAL, indexVal);
	}else{
		emitBundle(OP_GET_LOCAL, indexVal);
	}
}

static void namedGlobal(TK* id, bool canAssign, uint32_t index){
	Value indexVal = NUM_VAL(index);
	indexVal.charIndex = getTokenIndex(id->start);
	indexVal.line = id->line;

	if(canAssign && match(TK_ASSIGN)){
		expression();
		emitBundle(OP_DEF_GLOBAL, indexVal);
	}else{
		emitBundle(OP_GET_GLOBAL, indexVal);
	}
}

static void namedVariable(TK* id, bool canAssign){
	int32_t index = resolveLocal(id);
	if(index >= 0){
		namedLocal(id, canAssign, index);
	}else{
		namedGlobal(id, canAssign, getStringObjectIndex(id));
	}	
}

static void variable(bool canAssign){
	namedVariable(&parser.previous, canAssign);
}

static void markInitialized(){
	Compiler * comp = currentCompiler();
	if(comp->scopeDepth == 0) return;
	comp->locals[comp->localCount-1].depth = 
		comp->scopeDepth;
}

static void assignStatement(bool enforceGlobal){
	consume(TK_ID, "Expected an identifier.");
	Compiler* currentComp = currentCompiler();
	TK idToken = parser.previous;

	if(currentComp->scopeDepth > 0 && !enforceGlobal){

		for(int i = currentComp->localCount-1; i>=0; --i){
			Local currentVar = currentComp->locals[i];
			if(currentVar.depth > -1 && currentVar.depth < currentComp->scopeDepth){
				break;
			}
			if(tokensEqual(currentVar.id, currentComp->locals[i].id)){
				error("Variable has already been declared in this scope.");
			}
		}
		addLocal(currentCompiler(), idToken);
		parseAssignment();
		markInitialized(/*idToken*/);
	}else{
		parseAssignment();
		
		Value stringIndex = NUM_VAL(getStringObjectIndex(&idToken));
		stringIndex.charIndex = getTokenIndex(idToken.start);
		stringIndex.line = idToken.line;
		emitBundle(OP_DEF_GLOBAL, stringIndex);
	}
	endLine();
}

static void binary(bool canAssign){
	TKType op = parser.previous.type;

	ParseRule* rule = getRule(op);
	parsePrecedence((PCType) (rule->precedence + 1));

	switch(op){
		case TK_PLUS: 
			emitByte(OP_ADD); 
			break;
		case TK_MINUS: 
			emitByte(OP_SUBTRACT); 
			break;
		case TK_TIMES: 
			emitByte(OP_MULTIPLY); 
			break;
		case TK_DIVIDE: 
			emitByte(OP_DIVIDE); 
			break;
		case TK_MODULO: 
			emitByte(OP_MODULO); 
			break;
		case TK_LESS:
			emitByte(OP_LESS);
			break;
		case TK_GREATER:
			emitByte(OP_GREATER);
			break;
		case TK_LESS_EQUALS:
			emitByte(OP_LESS_EQUALS);
			break;
		case TK_GREATER_EQUALS:
			emitByte(OP_GREATER_EQUALS);
			break;
		case TK_EQUALS:
			emitByte(OP_EQUALS);
			break;
		case TK_BANG_EQUALS:
			emitBytes(OP_EQUALS, OP_NOT);
			break;
		default: 
			break;
	}
}

static void unary(bool canAssign){
	TKType op = parser.previous.type;

	parsePrecedence(PC_UNARY);

	switch(op){
		case TK_MINUS: emitByte(OP_NEGATE); break;
		case TK_BANG: emitByte(OP_NOT); break;
		default: return;
	}
}

static void grouping(bool canAssign){
	expression();
	consume(TK_R_PAREN, "Expect ')' after expression.");
}

static void printToken();

bool compile(char* source, CompilePackage* package){
	initScanner(source);
	Compiler comp;
	initCompiler(&comp, package);

	parser.hadError = false;
	parser.panicMode = false;
	parser.codeStart = source;
	compilingChunk = package->compiled;
	result = package;

	advance();
	while(parser.current.type != TK_EOF){
		statement();
	}

	consume(TK_EOF, "Expected end of expression.");
	endCompiler();

	#ifdef DEBUG
		if(!parser.hadError){
			printChunk(package->compiled, "result");
		}
	#endif
	return !parser.hadError;
}