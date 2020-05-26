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
CompilePackage* result = NULL;

#ifdef EM_MAIN
	extern void em_configureValuePointerOffsets(int* type, int* un, int* line, int* in);
	extern void em_addValue(int* inlineOffset, int* length, int* operator, Value* value);
	extern void em_addStringValue(char* charPtr, int inlineOffset, int length);
	extern void em_endLine(int* newlineIndex);
	extern void em_addUnlinkedValue(int* insertionIndex);

	void prepareValueConversion(){
		// a value might have a different memory padding depending on the compiler implementation, system, and emscripten version
		// so, the offsets are calculated each time the program is compiled to eliminate errors in converting values across the C/JS barrier
		Value val = NULL_VAL();
		void* base = (void*) &val;
		int type = (int) ((void*)&(val.type) - base);
		int as = (int) ((void*)&(val.as) - base);
		int lineIndex = (int) ((void*)&(val.lineIndex) - base);
		int inlineIndex = (int) ((void*)&(val.inlineIndex) - base);
		em_configureValuePointerOffsets(&type, &as, &lineIndex, &inlineIndex);
	}
#endif

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
	newComp->super = comp;

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

	if(chunkObj->chunkType == CK_CONSTR) newComp->enclosed = true;
	
	newComp->instanceType = TK_NULL;

	newComp->compilingChunk = chunkObj;
	newComp->returned = false;
	return newComp;
}

static void emitByte(uint8_t byte);
static void emitBytes(uint8_t byte1, uint8_t byte2);

static Value* emitConstant(Value v);
static void emitLinkedConstant(Value v, TK* token);

static void emitReturn(){
	ObjChunk* current = currentChunkObject();
	if(!currentCompiler()->returned){
		switch(current->chunkType){
			case CK_CONSTR:
				emitBytes(OP_POP_INST, (uint8_t) true);
				break;
			case CK_FUNC:
				emitConstant(NULL_VAL());
				break;
			default:
				break;
		}
		emitByte(OP_RETURN);
	}
}

static Compiler* exitCompilationScope(){
	Compiler* toFree = currentCompiler();
	Compiler* superComp = currentCompiler()->super;
	for(int i = 0; i<currentChunkObject()->numParameters; ++i){
		emitByte(OP_POP);
	}
	emitReturn();
	freeCompiler(toFree);
	return superComp;
}

static void enterScope(){
	++currentCompiler()->scopeDepth;
}

static void exitScope(){
	currentCompiler()->scopeDepth--;
	currentCompiler()->enclosed = false;
}

static void enterEnclosedScope(){
	++currentCompiler()->scopeDepth;
	currentCompiler()->enclosed = true;
}

static void exitEnclosedScope(){
	currentCompiler()->scopeDepth--;
	currentCompiler()->enclosed = false;
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

static void emitLinkedConstant(Value value, TK* token){
	#ifdef EM_MAIN
	if(!IS_NULL(value)){
		if(value.type == VL_OBJ && AS_OBJ(value)->type == OBJ_STRING){
			//em_addStringValue(AS_CSTRING(value), token->inlineIndex, token->length);
		}else{
			//em_addValue(token->inlineIndex, token->length);
		}
	}
	#endif
	writeConstant(currentChunk(), value, parser.previous.line);
}

static Value* emitConstant(Value value){
	return writeConstant(currentChunk(), value, parser.previous.line);
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
	if(parser.currentLineValueIndex != 0) {
		++parser.lineIndex;
		#ifdef EM_MAIN
			int distanceFromStart = (int) (parser.lastNewline - parser.codeStart);
			em_endLine(&distanceFromStart);
		#endif
		parser.currentLineValueIndex = 0;
	}
	parser.lastNewline = (parser.previous.start + parser.previous.length);
}

static Value getTokenStringValue(TK* token){
	Value strObj = OBJ_VAL(tokenString(token->start, token->length));
	return strObj;
}

static ObjString* getTokenStringObject(TK* token){
	return tokenString(token->start, token->length);
}

static uint32_t getStringObjectIndex(TK* token){
	if(token == NULL) return -1;
	return writeValue(currentChunk(), getTokenStringValue(token), parser.previous.line);
}

static uint32_t getObjectIndex(Obj* obj){
	if(obj == NULL) return -1;
	return writeValue(currentChunk(), OBJ_VAL(obj), parser.previous.line);
}

static void expression(bool emitTrace);
static void indentedBlock();

static void and_(bool canAssign);
static void binary(bool canAssign);
static void unary(bool canAssign);
static void grouping(bool canAssign);
static void number(bool canAssign);
static void literal(bool canAssign);
static void constant(bool canAssign);
static void constant(bool canAssign);
static void stringLiteral(bool canAssign);
static void array(bool canAssign);
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
	{ unary,	NULL,	    PC_UNARY },   // TK_INCR, 
	{ unary,	NULL,	    PC_UNARY },   // TK_DECR,
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
	{ stringLiteral,	NULL,	    PC_NONE },    // TK_STRING,
	{ variable, NULL,	    PC_NONE },    // TK_ID,
	{ constant, NULL,	    PC_NONE },    // TK_CONST,	
	{ NULL,	    NULL,	    PC_NONE },    // TK_FUNC,
	{ NULL,	    and_,	    PC_AND },     // TK_AND,
	{ NULL,	    NULL,	    PC_OR },    // TK_OR,
	{ NULL,	    NULL,	    PC_NONE },    // TK_PRE,
	{ native,	NULL,		PC_NONE },    // TK_SHAPE,
	{ native,	NULL,		PC_NONE },    // TK_NATIVE,
	{ NULL,	    NULL,	    PC_NONE },    // TK_SEMI,
	{ NULL,	    NULL,	    PC_NONE },    // TK_L_BRACE,
	{ NULL,	    NULL,	    PC_NONE },    // TK_R_BRACE,
	{ grouping, NULL,       PC_NONE },    // TK_L_PAREN,
	{ NULL,	    NULL,	    PC_NONE },    // TK_R_PAREN, 
	{ array,	NULL,	    PC_NONE },    // TK_L_BRACK,
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
	{ NULL,	    NULL,	    PC_NONE },    // TK_LET,
	{ NULL,	    NULL,	    PC_NONE },    // TK_VAR,
	{ NULL,	    NULL,	    PC_NONE },    // TK_PRINT,
	{ NULL,	    NULL,	    PC_NONE },    // TK_DRAW,
	{ NULL,	    NULL,	    PC_NONE },    // TK_TEXT,
	{ NULL,	    NULL,	    PC_NONE },    // TK_T,
	{ NULL,	    NULL,	    PC_NONE },    // TK_ERROR,
	{ NULL,	    NULL,	    PC_NONE },    // TK_EOF,
	{ NULL,	    NULL,	    PC_NONE },    // TK_AS,
	{ NULL,	    NULL,	    PC_NONE },    // TK_DEF,
	{ NULL,	    NULL,	    PC_NONE },    // TK_RET,
	{ NULL,	    NULL,	    PC_NONE },    // TK_REP,
	{ NULL,	    NULL,	    PC_NONE },    // TK_TO,
	{ NULL,	    NULL,	    PC_NONE },    // TK_FROM,
	{ NULL,	    NULL,	    PC_NONE },    // TK_WITH,
};

static void printStatement();
static void expressionStatement();
static void assignStatement(bool enforceGlobal);
static void defStatement();
static void frameStatement();
static void synchronize();
static uint8_t emitParams();
static void attr();
static void returnStatement();
static void repeatStatement();
static void withStatement();
static void whileStatement();
static void funcStatement();

static ParseRule* getRule(TKType type){
	ParseRule* rule = &rules[type];
	return rule;
}

static void parsePrecedence(PCType precedence){
	advance();
	ParseRule* prefixRule = getRule(parser.previous.type);

	if(prefixRule->prefix == NULL){
		errorAtCurrent("Expect expression.");
		return;
	}

	bool canAssign = (precedence <= PC_ASSIGN);
	prefixRule->prefix(canAssign);

	ParseRule* rule = getRule(parser.current.type);
	while(precedence <= rule->precedence){
		advance();
		ParseRule* infixRule = rule;
		if(infixRule){
			parser.lastOperator = parser.previous.type;
			parser.lastOperatorPrecedence = infixRule->precedence;
			infixRule->infix(canAssign);
		}
		rule = getRule(parser.current.type);
	}

	if(!canAssign && match(TK_ASSIGN)){
		error("Invalid assignment target.");
		expression(false);
	}
}

static void statement() {
	while(parser.current.type == TK_NEWLINE || parser.current.type == TK_INDENT){
		advance();
		if(parser.previous.type == TK_NEWLINE) 	parser.lastNewline = (parser.previous.start + parser.previous.length) - 1;;
	}
	if(parser.current.type != TK_EOF){
		switch(parser.current.type){
			case TK_WHILE:
				advance();
				whileStatement();
				break;
			case TK_DEF:
				advance();
				defStatement();
				break;
			case TK_FUNC:
				advance();
				funcStatement();
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
				}
				break;
			case TK_REP:
				advance();
				repeatStatement();
				break;
			case TK_WITH:
				advance();
				withStatement();
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
	while(parser.current.type != TK_EOF & parser.previous.type != TK_NEWLINE){
		switch(parser.current.type){
			case TK_PRINT:
			case TK_DRAW:
			case TK_LET:
				return;
			default: ;
		}
		advance();
	}
	if(parser.previous.type == TK_NEWLINE) parser.lastNewline = parser.previous.start;
}


static void expression(bool emitTrace) {
	parsePrecedence(PC_ASSIGN);
	if(emitTrace){
		if(parser.manipTarget){
			parser.manipTarget->lineIndex = parser.lineIndex;
			parser.manipTarget->inlineIndex = parser.currentLineValueIndex;
			#ifdef EM_MAIN
				int operator = (int)(parser.lastOperator);
				em_addValue(&parser.manipTargetCharIndex, &parser.manipTargetLength, &operator, parser.manipTarget);	
			#endif
			++parser.currentLineValueIndex;
		}else{
			int insertionIndex = parser.current.start - parser.lastNewline;
			#ifdef EM_MAIN
				em_addUnlinkedValue(&insertionIndex);
			#endif
		}
	}
	parser.manipTarget = NULL;
	parser.manipPrecedence = -1;
}
static void number(bool canAssign) {
	double value = strtod(parser.previous.start, NULL);
	Value val = NUM_VAL(value);
	Value* link = emitConstant(val);
	if((parser.lastOperatorPrecedence <= parser.manipPrecedence) && (parser.parenDepth == 0)){
		parser.manipTarget = link;
		parser.manipTargetCharIndex = parser.previous.start - parser.lastNewline;
		parser.manipTargetLength = parser.previous.length;
		parser.manipPrecedence = parser.lastOperatorPrecedence;
	}
}

static double tokenToNumber(TK token){
	if(token.type == TK_INTEGER || token.type == TK_REAL){
		return strtod(parser.previous.start, NULL);
	}else{
		return 0;
	}
}

static void literal(bool canAssign) {
	switch(parser.previous.type){
		case TK_FALSE:  emitByte(OP_FALSE); break;
		case TK_TRUE:   emitByte(OP_TRUE); break;
		case TK_NULL:   emitByte(OP_NULL); break;
		case TK_INTEGER:
		case TK_REAL:
			number(canAssign);
			return;
		default:
			break;
	}
}

static void returnStatement() {
	expression(true);
	emitByte(OP_RETURN);
	currentCompiler()->returned = true;
}

static int getIndentation() {
	while(parser.current.type == TK_INDENT){
		advance();
		if(parser.current.type != TK_NEWLINE){
			return parser.previous.length;
		}
		advance();
	}
	return 0;
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
				== currentComp->scopeDepth){
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

static void patchJump(uint32_t jumpIndex) {
	signed short backIndex = currentChunk()->count - (jumpIndex - 2);

	currentChunk()->code[jumpIndex] = ((backIndex >> 8) & 0xFF);
	currentChunk()->code[jumpIndex+1] = (backIndex & 0xFF);
}

static void jumpTo(uint8_t opcode, uint32_t opIndex) {
	Chunk* chunk = currentChunk();
	emitByte(opcode);
	signed short offset = (opIndex - 2) - chunk->count;
	emitByte((offset >> 8) & 0xFF);
	emitByte((offset) & 0xFF);
}

static void internGlobal(TK* id){
	add(currentResult()->globals, getTokenStringObject(id), NULL_VAL());
}

static int32_t resolveLocal(TK*id);
static int32_t resolveInstanceLocal();
static int32_t latestLocal();
static void markInitialized();
static void namedVariable(TK* id, bool canAssign);
static bool isInitialized(TK* id);
	
static void whileStatement(){
	uint32_t jumpIndex = currentChunk()->count;
	consume(TK_L_PAREN, "Expected '('.");
	expression(false);
	consume(TK_R_PAREN, "Expected ')'.");

	uint32_t endJumpIndex = emitJump(OP_JMP_FALSE);
	//execute the body
	endLine();

	enterScope();
	indentedBlock();	
	exitScope();

	jumpTo(OP_JMP, jumpIndex);
	patchJump(endJumpIndex);
}

static void repeatStatement() {
	uint32_t counterLocalIndex;
	if(parser.current.type == TK_ID || parser.current.type == TK_INTEGER){
		advance();

		TK counterToken = parser.previous;
		if(parser.previous.type == TK_ID){
			//if a local variable has been used, then its value is copied into a separate local.
			namedVariable(&counterToken, true);
		}else{
			emitConstant(NUM_VAL(tokenToNumber(counterToken)));
		}
		uint32_t counterLocalIndex = addCounterLocal(currentCompiler());
		
		endLine();

		//Mark the jump position to the before the decrement and repeat body
		uint32_t jumpIndex = currentChunk()->count;
		
		//decrement the counter by one
		emitBundle(OP_GET_LOCAL, counterLocalIndex);
		emitConstant(NUM_VAL(1));
		emitByte(OP_SUBTRACT);

		emitBundle(OP_DEF_LOCAL, counterLocalIndex);
		emitByte(OP_POP);

		//execute the body
		enterScope();
		indentedBlock();	
		exitScope();

		//jump back to the beginning if the counter has ended.
		emitBundle(OP_GET_LOCAL,counterLocalIndex);	
		emitConstant(NUM_VAL(1));
		emitByte(OP_LESS);
		//only jump back if the counter hasn't fallen below zero
		jumpTo(OP_JMP_FALSE, jumpIndex);

		//remove the counter local before moving on
		emitByte(OP_POP);
		--currentCompiler()->localCount;
	}else{
		errorAtCurrent("Expected an integer or identifier.");
	}
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

static uint8_t emitParameters(){
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
				if(paramCount + 1 <= 256){
					addLocal(currentCompiler(), parser.previous);
					markInitialized();
					++paramCount;
				}else{
					errorAtCurrent("Maximum parameter count (256) reached.");
					return paramCount;
				}
			}
		}
		consume(TK_R_PAREN, "Expected ')'.");
	}	
	return paramCount;
}

static void funcStatement(){
	Compiler* currentComp = currentCompiler();
	consume(TK_ID, "Expected an identifier.");
	if(!parser.hadError){
		TK funcIDToken = parser.previous;
		ObjString* funcName = getTokenStringObject(&funcIDToken);
		
		ObjChunk* newChunk = allocateChunkObject(funcName);
		newChunk->chunkType = CK_FUNC;
		compiler = enterCompilationScope(newChunk);

		uint8_t paramCount = emitParameters();
		if(!parser.hadError){
			newChunk->numParameters = paramCount;
			endLine();
			indentedBlock();
			
			compiler = exitCompilationScope();
			
			uint32_t scopeIndex = getObjectIndex((Obj*) newChunk);
			emitBundle(OP_CONSTANT, scopeIndex);
			if(currentCompiler()->scopeDepth > 0){	
				emitBundle(OP_DEF_LOCAL, resolveLocal(&funcIDToken));
			}else{
				emitBundle(OP_DEF_GLOBAL, getStringObjectIndex(&funcIDToken));
				internGlobal(&funcIDToken);
			}
			emitByte(OP_POP);
		}	
	}
}

static void defStatement() {
	Compiler* currentComp = currentCompiler();
	Chunk* superChunk = currentChunk();
	consume(TK_ID, "Expected an identifier.");

	TK idToken = parser.previous;
	TKType instanceType = TK_NULL;

	ObjString* funcName = getTokenStringObject(&idToken);

	ObjChunk* newChunk = allocateChunkObject(funcName);
	newChunk->chunkType = CK_CONSTR;

	compiler = enterCompilationScope(newChunk);

	uint8_t paramCount = emitParameters();
	newChunk->numParameters = paramCount;
	
	if(parser.current.type == TK_AS){
		advance();
		expression(false);
		emitByte(OP_PUSH_INST);
	}else{
		emitByte(OP_INIT_INST);
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
	emitByte(OP_POP);
}

static void withStatement(){
 	expression(false);
	endLine();
	emitByte(OP_PUSH_INST);

	enterEnclosedScope();
	indentedBlock();
	exitEnclosedScope();	

	emitBytes(OP_POP_INST, (uint8_t) false);
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

static void stringLiteral(bool canAssign) {
	emitLinkedConstant(getTokenStringValue(&parser.previous), &parser.previous);
}

static void array(bool canAssign) {
	uint32_t numParameters = 0;
	while(parser.current.type != TK_R_BRACK){
		expression(true);
		++numParameters;
		if(parser.current.type != TK_R_BRACK) consume(TK_COMMA, "Expected ','");
	}
	consume(TK_R_BRACK, "Expected ']'");
	emitBundle(OP_BUILD_ARRAY, numParameters);
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
		expression(false);
		if(!parser.panicMode) ++paramCount;
		while(parser.current.type == TK_COMMA){
			advance();
			expression(false);
			if(!parser.panicMode) ++paramCount;
		}
	}
	consume(TK_R_PAREN, "Expected ')'.");
	return paramCount;
}

static void printStatement() {
	consume(TK_L_PAREN, "Expected '('.");
	expression(false);
	consume(TK_R_PAREN, "Expected ')'.");
	emitByte(OP_PRINT);
	endLine();
}

static void expressionStatement() {
	expression(false);
	emitByte(OP_POP);
	endLine();
}

static void parseAssignment() {
	if(match(TK_ASSIGN)){
		expression(true);
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
		case CS_TRANSP:
			emitConstant(RGBA(0, 0, 0, 0));
			break;
		case CS_ERROR:
			errorAtCurrent("Invalid constant.");
			emitConstant(NULL_VAL());
			break;
	}
}

static void initNative(void* func, TK* id){
	ObjString* nativeString = tokenString(id->start, id->length);
	ObjNative* nativeObj = allocateNative(func);
	add(currentResult()->globals, nativeString, OBJ_VAL(nativeObj));
}

static void native(bool canAssign){
	TK nativeId = parser.previous;
	uint8_t numParams = emitParams();
	emitBundle(OP_GET_GLOBAL, getStringObjectIndex(&nativeId));
	emitBytes(OP_CALL, numParams);

	void* func;
	switch(nativeId.subtype){
		case TK_MOVE:
			func = move;
			break;
		case TK_VERT:
			func = vertex;
			break;
		case TK_ARC:
			func = arc;
			break;
		case TK_JUMP:
			func = jump;
			break;
		case TK_TURN:
			func = turn;
			break;
		case TK_RECT:
			func = rect;
			break;
		case TK_CIRC:
			func = circle;
			break;
		case TK_ELLIP:
			func = ellipse;
			break;
		case TK_LINE:
			func = line;
			break;
		case TK_PATH:
			func = path;
			break;
		case TK_POLY:
			func = polygon;
			break;
		case TK_POLYL:
			func = polyline;
			break;
		case TK_CBEZ:
			func = cBezier;
			break;
		case TK_QBEZ:
			func = qBezier;
			break;
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
		emitByte(OP_LOAD_INST);
		deref(canAssign);
	}else{
		errorAtCurrent("The current instance is null.");
	}
}

static void deref(bool canAssign){
	while(parser.previous.type == TK_DEREF){
		if(parser.current.type == TK_ID){
			advance();
			TK idToken = parser.previous;
			
			if(canAssign && match(TK_ASSIGN)){
				expression(true);
				emitBundle(OP_DEF_INST, getStringObjectIndex(&idToken));
				emitByte(0);
			}else{
				emitBundle(OP_DEREF, getStringObjectIndex(&parser.previous));
				if(parser.current.type == TK_DEREF) advance();
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

static int32_t resolveInstanceLocal(){
	Compiler* comp = currentCompiler();
	for(int i = comp->localCount-1; i>=0; --i){
		Local* currentLocal = &comp->locals[i];
		if(currentLocal->type == INST){
			return i;
		}
	}
	errorAtCurrent("Unable to dereference this scope.");
	return (int32_t) -1;
}

static int32_t latestLocal(){
	return currentCompiler()->localCount-1;
}
static void namedLocal(TK* id, bool canAssign, uint32_t index){
	if(canAssign && match(TK_ASSIGN)){
		expression(true);
		emitBundle(OP_DEF_LOCAL, index);
	}else{
		if(parser.current.type == TK_INCR || parser.current.type == TK_DECR){
			advance();
			int delta = (parser.previous.type == TK_INCR ? 1 : -1);
			emitConstant(NUM_VAL(delta));
			emitBundle(OP_GET_LOCAL, index);
			emitByte(OP_ADD);
			emitBundle(OP_DEF_LOCAL, index);
			emitByte(OP_POP);
		}else{
			emitBundle(OP_GET_LOCAL, index);
		}
	}
}

static void namedGlobal(TK* id, bool canAssign, uint32_t index){
	if(canAssign && match(TK_ASSIGN)){
		expression(true);
		emitBundle(OP_DEF_GLOBAL, index);
		internGlobal(id);
	}else{
		if(parser.current.type == TK_INCR || parser.current.type == TK_DECR){
			advance();
			int delta = (parser.previous.type == TK_INCR ? 1 : -1);
			emitConstant(NUM_VAL(delta));
			emitBundle(OP_GET_GLOBAL, index);
			emitByte(OP_ADD);
			emitBundle(OP_DEF_GLOBAL, index);
			emitByte(OP_POP);
		}else{
			emitBundle(OP_GET_GLOBAL, index);
		}
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
		int localIndex = latestLocal();
		emitBundle(OP_DEF_LOCAL, latestLocal());
	}else{
		parseAssignment();
		uint32_t stringIndex = getStringObjectIndex(&idToken);
		emitBundle(OP_DEF_GLOBAL, stringIndex);
		emitByte(OP_POP);
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
	switch(op){
		case TK_MINUS: parsePrecedence(PC_UNARY); emitByte(OP_NEGATE); break;
		case TK_BANG: parsePrecedence(PC_UNARY); emitByte(OP_NOT); break;
		case TK_DECR:
		case TK_INCR:{
			advance();
			if(parser.previous.type != TK_ID){
				errorAtCurrent("Expected an identifier.");
			}else{
				TK idToken = parser.previous;
				int32_t varIndex = resolveLocal(&idToken);
				uint8_t assignOp, getOp;

				if(varIndex >= 0){
					assignOp = OP_DEF_LOCAL;
					getOp = OP_GET_LOCAL;
					varIndex = (uint32_t) varIndex;
				}else{
					assignOp = OP_DEF_GLOBAL;
					getOp = OP_GET_GLOBAL;
					varIndex = getStringObjectIndex(&idToken);
				}

				int offset = (op == TK_DECR ? -1 : 1);
				emitConstant(NUM_VAL(offset));
				emitBundle(getOp, varIndex);
				emitByte(OP_ADD);
				emitBundle(assignOp, varIndex);
			}
		} break;
		default:
			break;
	}
}

static void grouping(bool canAssign){
	++parser.parenDepth;
	parsePrecedence(PC_ASSIGN);
	--parser.parenDepth;
	consume(TK_R_PAREN, "Expect ')' after expression.");
}

static void printToken();

void initParser(Parser* parser, char* source){
	parser->hadError = false;
	parser->panicMode = false;
	parser->codeStart = source;

	parser->lineIndex = 0;
	parser->currentLineValueIndex = 0;

	parser->lastNewline = source;

	parser->manipPrecedence = PC_PRIMARY;
	parser->lastOperator = TK_NULL;
	parser->lastOperatorPrecedence = PC_PRIMARY;

	parser->manipTarget = NULL;
	parser->manipTargetCharIndex = -1;
	parser->manipTargetLength = -1;
	parser->manipTargetParenDepth = -1;
	
	parser->parenDepth = 0;
}

bool compile(char* source, CompilePackage* package){
	#ifdef EM_MAIN
		prepareValueConversion();
	#endif

	initScanner(source);
	initParser(&parser, source);

	result = package;
	package->compiled = allocateChunkObject(NULL);
	package->compiled->chunkType = CK_MAIN;

	compiler = enterCompilationScope(package->compiled);

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