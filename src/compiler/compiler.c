#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

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
#include "natives.h"

Parser parser;
Compiler* compiler = NULL;
CompilePackage* result;

static Compiler* currentCompiler() {
	return compiler;
}

static ObjChunk* currentChunkObject() {
	return currentCompiler()->compilingChunk;
}

static Chunk* currentChunk() {
	return currentCompiler()->compilingChunk->chunk;
}

static Compiler* enclosingCompiler(){
	Compiler* comp = (Compiler*) currentCompiler()->super;
	return comp;
}

int getTokenIndex(char* tokenStart){
	return (unsigned) (tokenStart - parser.codeStart);
}

static Compiler* enterCompilationScope(ObjChunk* chunkObj){
	Compiler* newComp = ALLOCATE(Compiler, 1);
	Compiler* comp = currentCompiler();
	initMap(&newComp->classes);

	if(comp){
		mergeMaps(comp->classes, newComp->classes);
		newComp->scopeDepth = comp->scopeDepth + 1;
	}else{
		newComp->scopeDepth = 0;
	}

	newComp->scopeCapacity = 0;
	newComp->localCount = 0;
	newComp->locals = NULL;
	newComp->enclosed = true;
	newComp->instanceType = TK_NULL;
	newComp->super = comp;

	newComp->compilingChunk = chunkObj;
	return newComp;
}

static void emitByte(uint8_t byte);
static void emitConstant(Value v);

static void emitReturn(){
	ObjChunk* current = currentChunkObject();
	switch(current->chunkType){
		case CK_MAIN:
			emitByte(OP_RETURN);
			break;
		case CK_CONSTR:
			emitByte(OP_RETURN);
			break;
		case CK_UNDEF:
			current->chunkType = CK_CONSTR;
			emitByte(OP_RETURN);
			break;
		case CK_FUNC:
			break;
	}
}

static Compiler* exitCompilationScope(){
	Compiler* toFree = currentCompiler();
	Compiler* superComp = currentCompiler()->super;
	for(int i = 0; i<currentChunkObject()->numParameters; ++i){
		emitByte(OP_POP);
	}
	emitReturn();
	FREE(Compiler, toFree);
	return superComp;
}

static void enterScope(){
	currentCompiler()->scopeDepth++;
}

static void exitScope(){
	currentCompiler()->scopeDepth--;
}

CompilePackage* currentResult(){
	return result;
}

static void emitByte(uint8_t byte){
	writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2){
	emitByte(byte1);
	emitByte(byte2);
}

static void emitBundle(uint8_t opCode, uint32_t index){
	writeOperatorBundle(currentChunk(), opCode, index, parser.previous.line);
}

static void emitTriple(uint8_t opCode, uint32_t val1, uint32_t val2){
	writeChunk(currentChunk(), opCode, parser.previous.line);
	writeVariableData(currentChunk(), val1);
	writeVariableData(currentChunk(), val2);
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

static Value getTokenStringValue(TK* token){
	Value strObj = OBJ_VAL(internString(token->start, token->length));
	strObj.charIndex = getTokenIndex(token->start);
	strObj.line = token->line;
	return strObj;
}

static ObjString* getTokenStringObject(TK* token){
	return internString(token->start, token->length);
}

static uint32_t getStringObjectIndex(TK* token){
	if(token == NULL) return -1;
	return writeValue(currentChunk(), getTokenStringValue(token), parser.previous.line);
}

static uint32_t getObjectIndex(Obj* obj){
	if(obj == NULL) return -1;
	return writeValue(currentChunk(), OBJ_VAL(obj), parser.previous.line);
}

static void expression();
static void indentedBlock();

static void and_(bool canAssign);
static void binary(bool canAssign);
static void unary(bool canAssign);
static void grouping(bool canAssign);
static void number(bool canAssign);
static void literal(bool canAssign);
static void constant(bool canAssign);
static void constant(bool canAssign);
static void string(bool canAssign);
static void variable(bool canAssign);
static void deref(bool canAssign);
static void scopeDeref(bool canAssign);
static void native(bool canAssign);

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
	{ literal,  NULL,       PC_TERM },    // TK_REAL,
	{ literal,  NULL,       PC_TERM },    // TK_INTEGER,
	{ literal,	NULL,	    PC_TERM },    // TK_TRUE,
	{ literal,	NULL,	    PC_TERM },    // TK_FALSE,
	{ literal,	NULL,	    PC_TERM },    // TK_NULL,
	{ string,	NULL,	    PC_TERM },    // TK_STRING,
	{ variable, NULL,	    PC_TERM },    // TK_ID,
	{ constant, NULL,	    PC_TERM },    // TK_ID,	
	{ NULL,	    NULL,	    PC_NONE },    // TK_FUNC,
	{ native,	NULL,		PC_TERM },    // TK_SIN,
	{ native,	NULL,		PC_TERM },    // TK_COS,
	{ native,	NULL,		PC_TERM },    // TK_TAN,
	{ native,	NULL,		PC_TERM },    // TK_ASIN,
	{ native,	NULL,		PC_TERM },    // TK_ACOS,
	{ native,	NULL,		PC_TERM },    // TK_ATAN,
	{ native,	NULL,		PC_TERM },    // TK_HSIN,
	{ native,	NULL,		PC_TERM },    // TK_HCOS,
	{ native,	NULL,		PC_TERM },    // TK_DEG,
	{ native,	NULL,		PC_TERM },    // TK_RAD,
	{ native,	NULL,		PC_TERM },    // TK_SQRT,
	{ NULL,	    and_,	    PC_AND },     // TK_AND,
	{ NULL,	    NULL,	    PC_NONE },    // TK_OR,
	{ NULL,	    NULL,	    PC_NONE },    // TK_PRE,
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
static void returnStatement();
static void repeatStatement();

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

static void statement() {
	if(currentChunkObject()->chunkType == CK_FUNC){
		errorAtCurrent("Unreacheable code.");
	}
	while(parser.current.type == TK_NEWLINE || parser.current.type == TK_INDENT){
		advance();
	}
	if(parser.current.type != TK_EOF){
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
			case TK_RET:
				advance();
				if(currentChunkObject()->chunkType == CK_CONSTR){
					errorAtCurrent("Cannot return from a constructor.");
				}else{
					returnStatement();
					currentChunkObject()->chunkType = CK_FUNC;
				}
			case TK_REP:
				advance();
				repeatStatement();
				break;
			default:
				expressionStatement();
				break;
		}
		if(parser.panicMode){
			synchronize();
		}
	}
}

static void synchronize() {
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
		case TK_INTEGER:
		case TK_REAL:
			number(canAssign);
			break;
		default:
			return;
	}    
}

static void attr() {
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

static void returnStatement() {
	expression();
	emitByte(OP_RETURN);
}

static int getIndentation() {
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

static void indentedBlock() {
	int currentScopeDepth = currentCompiler()->scopeDepth;
	int initialLocalCount = currentCompiler()->localCount;
	while(parser.current.type != TK_EOF 
			&& getIndentation() >= currentScopeDepth
		){
		statement();
	}
	Compiler* currentComp = currentCompiler();
	while(currentComp->localCount > initialLocalCount
			&& currentComp->locals[currentComp->localCount-1].depth 
				> currentComp->scopeDepth){
		emitByte(OP_POP);
		--currentComp->localCount;
	}
}

static int emitJump(OpCode op) {
	emitByte(op);
	emitByte(0xFF);
	emitByte(0xFF);
	return currentChunk()->count - 2;
}

static void patchJump(int jumpIndex) {
	int16_t backIndex = currentChunk()->count - jumpIndex - 2;

	currentChunk()->code[jumpIndex] = (backIndex >> 8) & 0xFF;
	currentChunk()->code[jumpIndex + 1] = (backIndex) & 0xFF;
}

static void jumpTo(uint8_t opcode, int opIndex) {
	Chunk* chunk = currentChunk();
	emitByte(opcode);
	int16_t offset = opIndex - chunk->count;
	emitByte((offset >> 8) & 0xFF);
	emitByte(offset & 0xFF);
}

static void internGlobal(TK* id){
	add(currentResult()->globals, getTokenStringObject(id), NULL_VAL());
}

static int32_t resolveLocal(TK*id);
static int32_t latestLocal();
static void markInitialized();
static void namedVariable(TK* id, bool canAssign);
static bool isInitialized(TK* id);

static void repeatStatement() {
	bool variableDeclared = false;
	uint32_t counterLocalIndex = 0;
	int initialJumpIndex = 0;
	if(parser.current.type == TK_ID){
		advance();
		TK firstId = parser.previous;
		
		if(parser.current.type == TK_TO){
			int localIndex = resolveLocal(&parser.previous);

			if(localIndex == -1){
				counterLocalIndex = addLocal(currentCompiler(), parser.previous);
				emitConstant(NUM_VAL(0));
				markInitialized();

			}else{
				counterLocalIndex = localIndex;
			}
			advance();

			if(parser.current.type == TK_ID){
				TK maxId = parser.current;
	/*			if(!isInitialized(&maxId)){
					errorAtCurrent("Upper bound identifier is NULL.");
				}*/
				expression();
				int maxRangeLocalIndex = addDummyLocal(currentCompiler());
				endLine();		
				initialJumpIndex = currentChunk()->count-2;
				emitBundle(OP_GET_LOCAL,counterLocalIndex);
				namedVariable(&maxId, false);
				
			}else if (parser.current.type == TK_INTEGER){
				expression();
				int maxRangeLocalIndex = addDummyLocal(currentCompiler());
				endLine();		
				initialJumpIndex = currentChunk()->count-2;
				emitBundle(OP_GET_LOCAL,counterLocalIndex);
				emitBundle(OP_GET_LOCAL,maxRangeLocalIndex);
			}else{
				errorAtCurrent("Expected an integer or a defined identifier.");
			}
		}else{
			if(!isInitialized(&firstId)){
				errorAt(&firstId, "Upper bound identifier is NULL.");
			}
			counterLocalIndex = addDummyLocal(currentCompiler());
			emitConstant(NUM_VAL(0));
			endLine();		
			initialJumpIndex = currentChunk()->count-2;
			emitBundle(OP_GET_LOCAL,counterLocalIndex);
			namedVariable(&firstId, false);
		}

	}else{
		emitConstant(NUM_VAL(0));
		counterLocalIndex = addDummyLocal(currentCompiler());
		expression();
		endLine();
		int maxRangeLocalIndex = addDummyLocal(currentCompiler());
		initialJumpIndex = currentChunk()->count-2;
		emitBundle(OP_GET_LOCAL,counterLocalIndex);
		emitBundle(OP_GET_LOCAL, maxRangeLocalIndex);		
	}

	emitByte(OP_LESS);
	uint32_t jmpFalseLocation = emitJump(OP_JMP_FALSE);

	enterScope();
	indentedBlock();	
	exitScope();

	emitConstant(NUM_VAL(1));
	emitBundle(OP_GET_LOCAL, counterLocalIndex);
	emitByte(OP_ADD);

	emitBundle(OP_DEF_LOCAL, counterLocalIndex);
	emitByte(OP_POP);

	jumpTo(OP_JMP, initialJumpIndex);
	patchJump(jmpFalseLocation);
	emitBytes(OP_POP, OP_POP);
	currentCompiler()->localCount -= 2;
}

static void drawStatement() {
	Compiler* currentComp = currentCompiler();
	if(parser.current.type == TK_SHAPE){	//draw ___ <= rect, circle, etc.
		advance();
		ObjChunk* chunkObj = allocateChunkObject(NULL);
		chunkObj->chunkType = CK_UNDEF;
		chunkObj->instanceType = parser.previous.subtype;

		compiler = enterCompilationScope(chunkObj);
		indentedBlock();
		compiler = exitCompilationScope();

		uint32_t scopeIndex = getObjectIndex((Obj*) chunkObj);
		emitBundle(OP_CONSTANT, scopeIndex);
		emitBytes(OP_CALL, 0);
	}else{
		expression();
	}
	emitByte(OP_DRAW);
}


static void inherit(ObjChunk* currentChunk, uint8_t* numParams) {
	if(currentChunk){
		inherit(currentChunk->superChunk, numParams);
		int paramsConsumed = fmin(*numParams, currentChunk->numParameters);
		*numParams = *numParams - paramsConsumed;
		emitConstant(OBJ_VAL(currentChunk));
		emitBytes(OP_CALL, paramsConsumed);
	}
}

static ObjChunk* fromExpression() {
	consume(TK_ID, "Expected an identifier.");
	TK superclassId = parser.previous;
	ObjString* superclassStringObj = getTokenStringObject(&superclassId);
	Value superChunkVal = getValue(currentCompiler()->classes, superclassStringObj);
	ObjChunk* chunk = (ObjChunk*) AS_OBJ(superChunkVal);
	if(!IS_NULL(superChunkVal)){
		return AS_CHUNK(superChunkVal);
	}else{
		errorAtCurrent("Inherited object is not defined in this scope.");
	}
}

static void asExpression() {
	TK shapeToken = parser.current;
	advance();
	switch(shapeToken.type){
		case TK_SHAPE:
			currentChunkObject()->chunkType = CK_CONSTR;
			currentChunkObject()->instanceType = shapeToken.subtype;
			break;
		default:
			errorAt(&parser.current, "Expected a shape identifier.");
			break;
	}
}

static void defStatement() {
	Compiler* currentComp = currentCompiler();
	TKType instanceType = TK_NULL;
	CKType chunkType = CK_UNDEF;
	consume(TK_ID, "Expected an identifier.");

	TK idToken = parser.previous;
	ObjString* funcName = getTokenStringObject(&idToken);

	ObjChunk* newChunk = allocateChunkObject(funcName);
	compiler = enterCompilationScope(newChunk);

	//TODO: fix parameter overflow
	uint8_t paramCount = 0;
	if(parser.current.type == TK_L_PAREN){
		advance();
		if(parser.current.type != TK_R_PAREN){
			consume(TK_ID, "Expected an identifier.");
			addLocal(currentCompiler(), parser.previous);
			markInitialized();
			++paramCount;
			while(parser.current.type == TK_COMMA){
				advance();
				consume(TK_ID, "Expected an identifier.");
				addLocal(currentCompiler(), parser.previous);
				markInitialized();
				++paramCount;
			}
		}
		consume(TK_R_PAREN, "Expected ')'.");
	}
	newChunk->numParameters = paramCount;

	switch(parser.current.type){
		case TK_AS:
			advance();
			asExpression();
			if(parser.current.type == TK_FROM){
				advance();
				newChunk->superChunk = fromExpression();
			}
			break;
		case TK_FROM:
			advance();
			newChunk->superChunk = fromExpression();
			if(parser.current.type == TK_AS){
				advance();
				asExpression();
			}
			break;
		case TK_NEWLINE:
			break;
		default:
			errorAtCurrent("Unexpected token in function object definition.");
			break;
	}

	endLine();
	indentedBlock();
	compiler = exitCompilationScope();

	if(newChunk->chunkType == CK_CONSTR) {
		add(currentCompiler()->classes, getTokenStringObject(&idToken), OBJ_VAL(newChunk));
	}

	uint32_t scopeIndex = getObjectIndex((Obj*) newChunk);
	emitBundle(OP_CONSTANT, scopeIndex);
	if(currentCompiler()->scopeDepth > 0){	
		emitBundle(OP_DEF_LOCAL, resolveLocal(&idToken));

	}else{
		emitBundle(OP_DEF_GLOBAL, getStringObjectIndex(&idToken));
		internGlobal(&idToken);
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

	enterScope();
	indentedBlock();
	exitScope();

	patchJump(jumpIndex);
}

static void and_(bool canAssign) {
	int endJump = emitJump(OP_JMP_FALSE);
	emitByte(OP_POP);
	parsePrecedence(PC_AND);

	patchJump(endJump);
}

static void string(bool canAssign) {
	emitConstant(getTokenStringValue(&parser.previous));
}

static void timeStep(bool canAssign) {
	emitByte(OP_T);
}

static void ifStatement() {
}

static uint8_t emitParams() {
	uint8_t paramCount = 0;
	consume(TK_L_PAREN, "Expected '('.");
	if(parser.current.type != TK_R_PAREN){
		expression();
		if(!parser.panicMode) ++paramCount;
		while(parser.current.type == TK_COMMA){
			advance();
			expression();
			if(!parser.panicMode) ++paramCount;
		}
	}
	consume(TK_R_PAREN, "Expected ')'.");
	return paramCount;
}

static void printStatement() {
	consume(TK_L_PAREN, "Expected '('.");
	expression();
	consume(TK_R_PAREN, "Expected ')'.");
	emitByte(OP_PRINT);
	endLine();
}

static void dimsStatement() {
	uint8_t numParams = emitParams();
	emitBytes(OP_DIMS, numParams);
	endLine();
}

static void posStatement() {
	uint8_t numParams = emitParams();
	emitBytes(OP_POS, numParams);
	endLine();
}

static void expressionStatement() {
	expression();
	emitByte(OP_POP);
	endLine();
}

static void parseAssignment() {
	if(match(TK_ASSIGN)){
		expression();
	}else{
		emitByte(OP_NULL);
	}
}

static void constant(bool canAssign) {
	TK constId = parser.previous;
	CSType constType = (CSType) constId.subtype;
	switch(constType){
		case CS_PI:
			emitConstant(NUM_VAL(PI));
			break;
		case CS_TAU:
			emitConstant(NUM_VAL(2*PI));
			break;
		case CS_E:
			emitConstant(NUM_VAL(E));
			break;
		case CS_RED:
			emitConstant(RGB(255, 0, 0));
			break;
		case CS_ORANGE:
			emitConstant(RGB(255, 165, 0));
			break;
		case CS_YELLOW:
			emitConstant(RGB(255, 255, 0));
			break;
		case CS_GREEN:
			emitConstant(RGB(0, 128, 0));
			break;
		case CS_BLUE:
			emitConstant(RGB(0, 0, 255));
			break;
		case CS_PURPLE:
			emitConstant(RGB(128, 0, 128));
			break;
		case CS_BROWN:
			emitConstant(RGB(265, 42, 42));
			break;
		case CS_MAGENTA:
			emitConstant(RGB(255, 0, 255));
			break;
		case CS_OLIVE:
			emitConstant(RGB(128, 128, 0));
			break;
		case CS_MAROON:
			emitConstant(RGB(128, 0, 0));
			break;
		case CS_NAVY:
			emitConstant(RGB(0, 0, 128));
			break;
		case CS_AQUA:
			emitConstant(RGB(0, 255, 255));
			break;
		case CS_TURQ:
			emitConstant(RGB(64, 224, 208));
			break;
		case CS_SILVER:
			emitConstant(RGB(192, 192, 192));
			break;
		case CS_LIME:
			emitConstant(RGB(0, 255, 0));
			break;
		case CS_TEAL:
			emitConstant(RGB(0, 128, 128));
			break;
		case CS_INDIGO:
			emitConstant(RGB(75, 0, 130));
			break;
		case CS_VIOLET:
			emitConstant(RGB(238, 130, 238));
			break;
		case CS_PINK:
			emitConstant(RGB(255, 20, 147));
			break;
		case CS_BLACK:
			emitConstant(RGB(0, 0, 0));
			break;
		case CS_WHITE:
			emitConstant(RGB(255, 255, 255));
			break;
		case CS_GRAY:
			emitConstant(RGB(128, 128, 128));
			break;
		case CS_GREY:
			emitConstant(RGB(128, 128, 128));
			break;
		case CS_ERROR:
			errorAtCurrent("Invalid constant.");
			emitConstant(NULL_VAL());
			break;
	}
}

static void initNative(void* func, TK* id){
	ObjString* nativeString = internString(id->start, id->length);
	ObjNative* nativeObj = allocateNative(func);
	add(currentResult()->globals, nativeString, OBJ_VAL(nativeObj));
}

static void native(bool canAssign){
	TK nativeId = parser.previous;
	uint8_t numParams = emitParams();
	emitBundle(OP_GET_GLOBAL, getStringObjectIndex(&nativeId));
	emitBytes(OP_CALL, numParams);

	void* func;
	switch(nativeId.type){
		case TK_SIN:
			func = nativeSine;
			break;
		case TK_COS:
			func = nativeCosine;
			break;
		case TK_TAN:
			func = nativeTangent;
			break;
		case TK_ASIN:
			func = nativeArcsin;
			break;
		case TK_ACOS:
			func = nativeArccos;
			break;
		case TK_ATAN:
			func = nativeArctan;
			break;
		case TK_HSIN:
			func = nativeHypsin;
			break;
		case TK_HCOS:
			func =  nativeHypcos;
			break;
		case TK_DEG:
			func = nativeDegrees;
			break;
		case TK_RAD:
			func = nativeRadians;
			break;
		case TK_SQRT:
			func = nativeSqrt;
			break;
		default:
			break;
	}
	initNative(func, &nativeId);
}

static void scopeDeref(bool canAssign){
	if(currentCompiler()->enclosed){
		emitByte(OP_LOAD_INSTANCE);
		deref(canAssign);
	}else{
		errorAtCurrent("Cannot set a scope property outside of a scope");
	}
}

static void deref(bool canAssign){
	while(parser.previous.type == TK_DEREF){
		if(parser.current.type == TK_ID){
			advance();
			TK idToken = parser.previous;
			
			if(canAssign && match(TK_ASSIGN)){
				expression();
				emitBundle(OP_DEF_INST, getStringObjectIndex(&idToken));
			}else{
				emitBundle(OP_DEREF, getStringObjectIndex(&parser.previous));
				if(parser.current.type == TK_DEREF) advance();
			}
		}else{
			errorAtCurrent("Expected an identifier.");
		}
	}
	/*
	while(parser.previous.type == TK_DEREF){
		TK possibleID = parser.current;
		if(possibleID.type == TK_ID){
			advance();
			if(parser.current.type == TK_DEREF){
				uint32_t idIndex = getStringObjectIndex(&parser.previous);
				emitBundle(OP_GET_SCOPE, idIndex);
				advance();
			}else{
				//reached the end of the deref chain; determing whether an assignment is needed.
				uint32_t idIndex = getStringObjectIndex(&parser.previous);
				uint32_t encloseIndex = getStringObjectIndex(&parser.lastID);

				if(canAssign && match(TK_ASSIGN)){
					expression();
					emitTriple(OP_DEF_SCOPE, idIndex, encloseIndex);
				}else{
					emitBundle(OP_GET_SCOPE, idIndex);
				}
			}
		}else{
		}
	}*/
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

static int32_t latestLocal(){
	return currentCompiler()->localCount-1;
}
static void namedLocal(TK* id, bool canAssign, uint32_t index){
	if(canAssign && match(TK_ASSIGN)){
		expression();
		emitBundle(OP_DEF_LOCAL, index);
	}else{
		emitBundle(OP_GET_LOCAL, index);
	}
}

static void namedGlobal(TK* id, bool canAssign, uint32_t index){
	if(canAssign && match(TK_ASSIGN)){
		expression();
		emitBundle(OP_DEF_GLOBAL, index);
		internGlobal(id);
	}else{
		emitBundle(OP_GET_GLOBAL, index);
	}
}

static bool isGloballyDefined(TK* id){
	return findKey(currentResult()->globals, id->start, id->length) != NULL;
}

static bool isInitialized(TK* id){
	return isGloballyDefined(id) || (resolveLocal(id) >= 0);
}

static void namedVariable(TK* id, bool canAssign){
	int32_t index = resolveLocal(id);
	if(index >= 0){
		namedLocal(id, canAssign, index);
	}else{
		namedGlobal(id, canAssign, getStringObjectIndex(id));
	}	
}

static void functionCall(){}

static void variable(bool canAssign){
	if(parser.current.type == TK_L_PAREN){
		TK funcName = parser.previous;
		Value classValue = getValue(currentCompiler()->classes, getTokenStringObject(&funcName));
		uint8_t numParams = 0;
		if(!IS_NULL(classValue)){
			numParams = emitParams();
			inherit(AS_CHUNK(classValue), &numParams);
		}else{
			numParams = emitParams();
			namedVariable(&funcName, canAssign);
			emitBytes(OP_CALL, numParams);
		}

	}else{
		namedVariable(&parser.previous, canAssign);
	}
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
			if(tokensEqual(currentVar.id, idToken)){
				error("Variable has already been declared in this scope.");
			}
		}
		addLocal(currentCompiler(), idToken);
		parseAssignment();
		markInitialized(/*idToken*/);
		emitBundle(OP_DEF_LOCAL, latestLocal());
	}else{
		parseAssignment();
		
		uint32_t stringIndex = getStringObjectIndex(&idToken);

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
	compiler = enterCompilationScope(package->compiled);

	parser.hadError = false;
	parser.panicMode = false;
	parser.codeStart = source;
	result = package;

	advance();
	while(parser.current.type != TK_EOF){
		statement();
	}

	consume(TK_EOF, "Expected end of expression.");
	compiler = exitCompilationScope();

	#ifdef DEBUG
		if(!parser.hadError){
			printChunk(package->compiled->chunk, "result");
		}
	#endif
	return !parser.hadError;
}