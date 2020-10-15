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
#include "tokenizer.h"
#include "parsemap.h"

Parser parser;
Compiler* compiler = NULL;
CompilePackage* result = NULL;

#ifdef EM_MAIN
	extern void em_configureValuePointerOffsets(int* type, int* un, int* line, int* in);
	extern void em_addValue(int* inlineOffset, int* length, int* operator, Value* value);
	extern void em_addStringValue(char* charPtr, int inlineOffset, int length);
	extern void em_endLine(int* newlineIndex);
	extern void em_addUnlinkedValue(int* insertionIndex, Value* value);
	extern void em_setMaxFrameIndex(uint32_t maxFrameIndex);
	extern void em_addThunkToInterval(unsigned long thunkPointer);
	extern void em_setInterval(int lower, int upper);
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
		newComp->animUpperBound = comp->animUpperBound;
		newComp->animLowerBound = comp->animLowerBound;
		newComp->animated = comp->animated;
	}else{
		newComp->scopeDepth = 0;
		newComp->animUpperBound = 0;
		newComp->animLowerBound = 0;
		newComp->animated = false;
	}

	newComp->scopeCapacity = 0;
	newComp->localCount = 0;
	newComp->locals = NULL;

	newComp->upvalues = NULL;
	newComp->upvalueCapacity = 0;

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
	emitConstant(value);
}

static Value* emitConstant(Value value){
	emitByte(OP_CONSTANT);
	return &(currentChunk()->constants->values[writeConstant(currentChunk(), value, parser.previous.line)]);
	
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

static bool consume(TKType type, char* message){
	if(parser.current.type == type){
		advance();
		return true;
	}
	errorAtCurrent(message);
	return false;
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

static Compiler* exitCompilationScope(){
	Compiler* toFree = currentCompiler();
	Compiler* superComp = currentCompiler()->super;
	for(int i = 0; i<currentChunkObject()->numParameters; ++i){
		emitByte(OP_POP);
	}
	emitReturn();
	if(superComp){
		uint32_t scopeIndex = writeValue(superComp->compilingChunk->chunk, OBJ_VAL(toFree->compilingChunk), parser.previous.line);
		writeOperatorBundle(superComp->compilingChunk->chunk, OP_CLOSURE, scopeIndex, parser.current.line);
		for (int i = 0; i < toFree->compilingChunk->upvalueCount; i++) {
			writeChunk(superComp->compilingChunk->chunk, toFree->upvalues[i].isLocal ? 1 : 0, parser.current.line);
			writeVariableData(superComp->compilingChunk->chunk, toFree->upvalues[i].index);
		}
	}
	freeCompiler(toFree);
	return superComp;
}

static int32_t resolveLocal(Compiler* comp, TK*id){
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

static bool isGloballyDefined(TK* id){
	return findKey(currentResult()->globals, id->start, id->length) != NULL;
}

static bool isInitialized(TK* id){
	return isGloballyDefined(id) || (resolveLocal(currentCompiler(), id) >= 0);
}

static void expression(bool emitTrace);
static void thunkExpression(bool emitTrace);
static void indentedBlock();
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
static void forStatement();
static void withStatement();
static void whileStatement();
static void funcStatement();
static void animStatement();
static void toStatement();

static ParseRule* getRule(TKType type){
	uint32_t typeAsInt = (uint32_t) type;
	if(typeAsInt < NUM_PARSE_RULES){
		ParseRule* rule = &rules[type];
		return rule;
	}else{
		return NULL;
	}

}

static void parsePrecedence(PCType precedence){
	advance();
	ParseRule* prefixRule = getRule(parser.previous.type);
	if(prefixRule == NULL || prefixRule->prefix == NULL){
		errorAtCurrent("Expect expression.");
	}else{
		bool canAssign = (precedence <= PC_ASSIGN);
		prefixRule->prefix(canAssign);

		ParseRule* rule = getRule(parser.current.type);
	
		while(rule && precedence <= rule->precedence){
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
			case TK_RETURN:
				advance();
				if(currentChunkObject()->chunkType == CK_CONSTR){
					errorAtCurrent("Cannot return from a constructor.");
				}else{
					returnStatement();
				}
				break;
			case TK_REPEAT:
				advance();
				repeatStatement();
				break;
			case TK_ANIM:
				advance();
				animStatement();
				break;
			case TK_FOR:
				advance();
				forStatement();
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

static void thunkExpression(bool emitTrace){		
	ObjChunk* newChunk = allocateChunkObject(NULL);
	newChunk->chunkType = CK_FUNC;
	compiler = enterCompilationScope(newChunk);
	expression(emitTrace);
	compiler = exitCompilationScope();

	if(currentCompiler()->animated){
		#ifdef EM_MAIN
			em_addThunkToInterval((unsigned long) newChunk);
		#endif
	}
	emitConstant(OBJ_VAL(newChunk));
}

static void expression(bool emitTrace) {
	parsePrecedence(PC_ASSIGN);
	if(emitTrace){
			if(parser.manipTarget){
				parser.manipTarget->lineIndex = parser.lineIndex;
				parser.manipTarget->inlineIndex = parser.currentLineValueIndex;
				int operator = (int)(parser.lastOperator);
				#ifdef EM_MAIN
					em_addValue(&parser.manipTargetCharIndex, &parser.manipTargetLength, &operator, parser.manipTarget);	
				#endif
			}else{
				if(parser.lastValueEmitted){
					parser.lastValueEmitted->lineIndex = parser.lineIndex;
					parser.lastValueEmitted->inlineIndex = parser.currentLineValueIndex;
					int insertionIndex = parser.current.start + parser.current.length - parser.lastNewline;
					#ifdef EM_MAIN
						em_addUnlinkedValue(&insertionIndex, parser.lastValueEmitted);
					#endif
				}
			}
		++parser.currentLineValueIndex;
	}
	parser.manipTarget = NULL;
	parser.lastValueEmitted = NULL;
	parser.manipTargetLength = 0;
	parser.manipTargetCharIndex = -1;
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
	parser.lastValueEmitted = link;
}

static double tokenToNumber(TK token){
	if(token.type == TK_INTEGER || token.type == TK_REAL){
		return strtod(parser.previous.start, NULL);
	}else{
		return 0;
	}
}

void literal(bool canAssign) {
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
	if(parser.current.type == TK_INDENT){
		return parser.current.length;
	}
	return 0;
}

static void indentedBlock() {
	int currentScopeDepth = currentCompiler()->scopeDepth;
	int initialLocalCount = currentCompiler()->localCount;
	while(parser.current.type != TK_EOF 
			&& getIndentation() >= currentScopeDepth
		){
		advance();
		statement();
	}
	Compiler* currentComp = currentCompiler();
	while(currentComp->localCount > initialLocalCount
			&& currentComp->locals[currentComp->localCount-1].depth == currentComp->scopeDepth){
		if(currentComp->locals[currentComp->localCount-1].isCaptured){
			emitByte(OP_CLOSE_UPVALUE);
		}else{
			emitByte(OP_POP);
		}
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
	signed short backIndex = currentChunk()->count - jumpIndex - 2;
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

static void markInitialized();
static void namedVariable(TK* id, bool canAssign);
	
static void animStatement(){
	consume(TK_ID, "Expected an identifier.");
	if(!parser.hadError){
		TK animToken = parser.previous;			
		int lowerBound = 0;
		int upperBound = 999;
		endLine();
		int currentScopeDepth = currentCompiler()->scopeDepth;
		while(parser.current.type != TK_EOF 
				&& getIndentation() >= currentScopeDepth
			){
			int lowerBound = 0;
			int upperBound = 999;
			//there are four possible cases for handling animation: to x, from y, to x from y, from y to x, and at x
			advance();
			switch(parser.previous.type){
				case TK_AT: {
					if(parser.current.type != TK_INTEGER) {
						print(O_ERR, "Expected an integer.");
					}else{
						upperBound = tokenToNumber(parser.current);
						advance();
					}
				} break;
				case TK_FROM:{
					if(parser.current.type != TK_INTEGER){
						print(O_ERR, "Expected an integer lower bound.");
					}else{
						lowerBound = tokenToNumber(parser.current);
						advance();
						if(parser.current.type == TK_TO){
							advance();
							if(parser.current.type != TK_INTEGER){
								print(O_ERR, "Expected an integer upper bound.");
							}else{
								upperBound = tokenToNumber(parser.current);
								advance();
							}
						}
					}
				} break;
				case TK_TO:{
					if(parser.current.type != TK_INTEGER){
						print(O_ERR, "Expected an integer upper bound.");
					}else{
						upperBound = tokenToNumber(parser.current);
						advance();
						if(parser.current.type == TK_FROM){
							advance();
							if(parser.current.type != TK_INTEGER){
								print(O_ERR, "Expected an integer lower bound.");
							}else{
								lowerBound = tokenToNumber(parser.current);
								advance();
							}
						}
					}					
				}break;
				default:{
					print(O_ERR, "Expected a to, from, or at statement.");
				} break;
			}
			endLine();

			#ifdef EM_MAIN
			em_setInterval(lowerBound, upperBound);
			#endif

			currentCompiler()->animated = true;
			enterScope();	
			indentedBlock();
			exitScope();
			currentCompiler()->animated = false;
		}
	}
}

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
		emitBundle(OP_GET_LOCAL, counterLocalIndex);	
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

static void forStatement() {
	if(consume(TK_ID, "Expected an identifier.")){
		TK counterToken = parser.previous;
		uint32_t varIndex = 0;
		OpCode getCode = OP_GET_LOCAL;
		OpCode defCode = OP_DEF_LOCAL;
		bool wasDefinedInOuterScope = false;

		if(parser.current.type == TK_ASSIGN){
			
			advance();
			expression(false);
			addLocal(currentCompiler(), counterToken);
			varIndex = latestLocal();
			emitBundle(OP_DEF_LOCAL, latestLocal());

		}else{
			bool wasDefinedInOuterScope = true;
			if(isGloballyDefined(&counterToken)){
				getCode = OP_GET_GLOBAL;
				defCode = OP_DEF_GLOBAL;
				varIndex = getStringObjectIndex(&counterToken);
			}else if((varIndex = resolveLocal(currentCompiler(), &counterToken)) >= 0){
			}else{
				errorAt(&counterToken, "Expected an initialized variable or an initialization expression.");
				return;
			}
		}

		if(parser.current.type == TK_TO){
			advance();
			if(consume(TK_INTEGER, "Expected an integer bound.")){
				uint32_t boundValue = tokenToNumber(parser.previous);
				uint32_t incrementValue = 1;
				if(consume(TK_BY, "Expected 'by'.")){
					if(consume(TK_INTEGER, "Expected an integer increment value.")){
						incrementValue = tokenToNumber(parser.previous);
						endLine();	

						uint32_t jumpBackIndex = currentChunk()->count;

						emitBundle(getCode, varIndex);
						emitConstant(NUM_VAL(boundValue));
						emitByte(OP_LESS);
						uint32_t jumpIndex = emitJump(OP_JMP_FALSE);

						//execute the body
						enterScope();
						indentedBlock();	
						exitScope();

						emitBundle(getCode, varIndex);
						emitConstant(NUM_VAL(incrementValue));
						emitByte(OP_ADD);
						emitBundle(defCode, varIndex);
						emitByte(OP_POP);

						jumpTo(OP_JMP, jumpBackIndex);

						patchJump(jumpIndex);
						if(!wasDefinedInOuterScope) {
							emitByte(OP_POP);
							--currentCompiler()->localCount;
						}
					}	
				}
			}
		}else if(parser.current.type == TK_IN){
			advance();
		}else{
			errorAtCurrent("Expected 'to' or 'in'.");
		}		
	}else{
		errorAtCurrent("Expected an initialized variable or an initialization expression.");
	}
}

static void inherit(ObjChunk* currentChunk, uint8_t* numParams) {
	if(currentChunk){
		inherit(currentChunk->superChunk, numParams);
		uint8_t paramsConsumed = (uint8_t) fmin(*numParams, currentChunk->numParameters);
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

			if(currentCompiler()->scopeDepth > 0){	
				emitBundle(OP_DEF_LOCAL, addLocal(currentCompiler(), funcIDToken));
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
	
	if(currentCompiler()->scopeDepth > 0){	
		emitBundle(OP_DEF_LOCAL, resolveLocal(currentCompiler(), &idToken));
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

void and_(bool canAssign) {
	int endJump = emitJump(OP_JMP_FALSE);
	emitByte(OP_POP);
	parsePrecedence(PC_AND);

	patchJump(endJump);
}

void stringLiteral(bool canAssign) {
	emitLinkedConstant(getTokenStringValue(&parser.previous), &parser.previous);
}

void array(bool canAssign) {
	uint32_t numParameters = 0;
	while(parser.current.type != TK_R_BRACK){
		expression(true);
		++numParameters;
		if(parser.current.type != TK_R_BRACK) consume(TK_COMMA, "Expected ','");
	}
	consume(TK_R_BRACK, "Expected ']'");
	emitBundle(OP_BUILD_ARRAY, numParameters);
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


#define E (2.718281828459045235360)
#define PI (3.141592653589793238462)
void constant(bool canAssign) {
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
		case CS_TURQUOISE:
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
		case CS_TRANSPARENT:
			emitConstant(RGBA(0, 0, 0, 0));
			break;
		case CS_X:
			emitConstant(NUM_VAL((int) CS_X));
			break;
		case CS_Y:
			emitConstant(NUM_VAL((int) CS_Y));
			break;
		case CS_XY:
			emitConstant(NUM_VAL((int) CS_XY));
			break;
		case CS_CENTER:
			emitConstant(VECTOR(0, 0));
			break;
		case CS_LCORNER:
			emitConstant(VECTOR(0, 0));
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

void native(bool canAssign){
	TK nativeId = parser.previous;
	uint8_t numParams = emitParams();
	emitBundle(OP_GET_GLOBAL, getStringObjectIndex(&nativeId));
	emitBytes(OP_CALL, numParams);

	void* func;
	switch(nativeId.subtype){
		case TK_MOVE:
			func = move;
			break;
		case TK_VERTEX:
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
		case TK_CIRCLE:
			func = circle;
			break;
		case TK_ELLIPSE:
			func = ellipse;
			break;
		case TK_LINE:
			func = line;
			break;
		case TK_PATH:
			func = path;
			break;
		case TK_POLYGON:
			func = polygon;
			break;
		case TK_UNGON:
			func = ungon;
			break;
		case TK_POLYLINE:
			func = polyline;
			break;
		case TK_CBEZIER:
			func = cBezier;
			break;
		case TK_QBEZIER:
			func = qBezier;
			break;
		case TK_MIRROR:
			func = mirror;
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
		case TK_RAND:
			func = nativeRandom;
			break;
		default:
			break;
	}
	initNative(func, &nativeId);
}

void deref(bool canAssign){
	while(parser.previous.type == TK_DEREF){
		if(parser.current.type == TK_ID){
			advance();
			TK idToken = parser.previous;
			
			if(canAssign && match(TK_ASSIGN)){
				if(currentCompiler()->animated){
					thunkExpression(true);
				}else{
					expression(true);
				}
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


void scopeDeref(bool canAssign){
	if(currentCompiler()->enclosed){
		emitByte(OP_LOAD_INST);
		deref(canAssign);
	}else{
		errorAtCurrent("The current instance is null.");
	}
}

static int resolveUpvalue(Compiler* compiler, TK* id) {
	if(compiler->super == NULL) return -1;

	int local = resolveLocal(compiler->super, id);
	if(local >= 0){
		compiler->super->locals[local].isCaptured = true;
		return addUpvalue(compiler, local, true);
	}

	int upvalue = resolveUpvalue(compiler->super, id);
	if(upvalue >= 0){
		return addUpvalue(compiler, upvalue, false);
	}

	return -1;
}

static void namedVariable(TK* id, bool canAssign){
	int32_t index = resolveLocal(currentCompiler(), id);
	uint8_t getOp;
	uint8_t defOp;

	if(index >= 0){
		getOp = OP_GET_LOCAL;
		defOp = OP_DEF_LOCAL;
	}else if ((index = resolveUpvalue(currentCompiler(), id)) >= 0){
		getOp = OP_GET_UPVALUE;
		defOp = OP_DEF_UPVALUE;
	}else{
		index = getStringObjectIndex(id);
		getOp = OP_GET_GLOBAL;
		defOp = OP_DEF_GLOBAL;
	}	

	if(canAssign && match(TK_ASSIGN)){
		expression(true);
		emitBundle(defOp, index);
	}else{
		if(parser.current.type == TK_INCR || parser.current.type == TK_DECR){
			advance();
			int delta = (parser.previous.type == TK_INCR ? 1 : -1);
			emitConstant(NUM_VAL(delta));
			emitBundle(getOp, index);
			emitByte(OP_ADD);
			emitBundle(defOp, index);
			emitByte(OP_POP);
		}else{
			emitBundle(getOp, index);
		}
	}
}

void variable(bool canAssign){
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
		internGlobal(&idToken);
		parseAssignment();
		uint32_t stringIndex = getStringObjectIndex(&idToken);
		emitBundle(OP_DEF_GLOBAL, stringIndex);
		emitByte(OP_POP);
  	}
	endLine();
}

void binary(bool canAssign){
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

void unary(bool canAssign){
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
				int32_t varIndex = resolveLocal(currentCompiler(), &idToken);
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

void grouping(bool canAssign){
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
	parser->manipTargetLength = 0;
	
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

	result->upperLimit = compiler->animUpperBound;
	compiler = exitCompilationScope();

	#ifdef DEBUG
		if(!parser.hadError){
			printChunk(package->compiled->chunk, "result");
		}
	#endif
	return !parser.hadError;
}