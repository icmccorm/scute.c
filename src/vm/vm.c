#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include "common.h"
#include "vm.h"
#include "value.h"
#include "debug.h"
#include "compiler.h"
#include "obj.h"
#include "hashmap.h"
#include "output.h"
#include "svg.h"
#include "package.h"

#ifdef EM_MAIN
	extern void setMaxFrameIndex(int index);
#endif

VM vm;

static void resetStack(){
	vm.stackTop = vm.stack;
}

void initVM(CompilePackage* code, int frameIndex) {
	resetStack();
	initMap(&vm.globals);
	vm.frameIndex = frameIndex;
	vm.chunk = code->compiled;
	vm.ip = vm.chunk->code;
	vm.runtimeObjects = NULL;
	vm.stackSize = 0;
	vm.currentScope = NULL;
	vm.currentFrame = NULL;
}

void freeVM() {
	freeMap(vm.globals);
	freeObjects(vm.runtimeObjects);
	vm.chunk = NULL;
}

void push(Value value) {
	*vm.stackTop = value;
	vm.stackTop++;
}

int stackSize(){
	return vm.stackTop - vm.stackBottom;
}

Value pop(){
	vm.stackTop--;
	return *vm.stackTop;
}

static Value peek(int distance){
	return vm.stackTop[-1 - distance];
}

static void runtimeError(char* format, ...){
	size_t opIndex = vm.ip - vm.chunk->code;
	int line = getLine(vm.chunk, opIndex);
	print(O_ERR, "[line %d] ", line);
	
	va_list args;
	va_start(args, format);
	vprint(O_ERR, format, args);
	print(O_ERR, "\n");
	va_end(args);
	resetStack();
}

static bool isFalsey(Value val){
	return IS_NULL(val) || (IS_BOOL(val) && !AS_BOOL(val));
}

static bool valuesEqual(Value a, Value b){
	if(a.type != b.type) return false;
	switch(a.type){
		case VL_BOOL:
			return AS_BOOL(a) == AS_BOOL(b);
		case VL_NULL:
			return true;
		case VL_OBJ:
			return AS_OBJ(a) == AS_OBJ(b);
		case VL_NUM:
			return AS_NUM(a) == AS_NUM(b);
	}	
}

static InterpretResult executeChunk(Chunk* chunk);

#define READ_BYTE() (*vm.ip++)
static uint32_t readInteger() {
	uint8_t numBytes = READ_BYTE();
	uint32_t index = 0; 
	for(int i = 0; i<numBytes; ++i) { 
		index = index | (READ_BYTE() << (8*i)); 
	}
	return index;
}

static InterpretResult run() {

#ifdef DEBUG
	for(Value* slot = vm.stack; slot < vm.stackTop; slot++){
		print(O_DEBUG, "[ ");
		printValue(O_DEBUG, *slot);
		print(O_DEBUG, " ]");
	}
	print(O_DEBUG, "\n");
#endif

#define READ_BYTE() (*vm.ip++)
#define READ_SHORT() (vm.ip += 2, (uint16_t)((vm.ip[-2] << 8) | vm.ip[-1]))
#define READ_CONSTANT() (vm.chunk->constants.values[readInteger()])
#define CONSTANT(index) (vm.chunk->constants.values[index])
#define BINARY_OP(op, valueType, operandType) \
	do { \
		if(!IS_NUM(peek(0)) || !IS_NUM(peek(1))){ \
			runtimeError("Operands must be numbers"); \
			return INTERPRET_RUNTIME_ERROR; \
		} \
		operandType b = AS_NUM(pop()); \
		operandType a = AS_NUM(pop()); \
		push(valueType(a op b)); \
	} while(false);

	for(;;) {
		Value a;
		Value b;
		switch(READ_BYTE()){
			case OP_DRAW: {
				ObjScope* shape = AS_SCOPE(pop());
				shape->nextShape = vm.currentFrame;
				vm.currentFrame = shape;
			} break;
			case OP_DEREF: {
				uint32_t valIndex = readInteger();
				Value superString = CONSTANT(valIndex-1);
				Value idString = CONSTANT(valIndex);

				Value closeVal = pop();
				if(IS_NULL(closeVal)) runtimeError("Cannot dereference null scope '%s'.", AS_STRING(superString));

				ObjScope* scope = (ObjScope*) AS_OBJ(closeVal);
				Value innerVal = getValue(scope->map, AS_STRING(idString));
				push(innerVal);
			} break;
			case OP_SCOPE: {
				//def "id"
				ObjString* idString = AS_STRING(READ_CONSTANT());
				//from "super"
				ObjString* superString = AS_STRING(READ_CONSTANT());
				//as "shape"
				Value shapeType = NUM_VAL(READ_BYTE());
				ObjScope* close = allocateShapeScope(shapeType);
				insert(vm.globals, idString, OBJ_VAL(close));
				
				ObjScope* previousClose = vm.currentScope;
				vm.currentScope = close;

				Chunk* scopeChunk = AS_CHUNK(READ_CONSTANT());
				executeChunk(scopeChunk);

				Value closeVal = OBJ_VAL(close);
				push(closeVal);

				vm.currentScope = previousClose;
			} break;
			
			case OP_JMP_FALSE: ;
				uint16_t offset = READ_SHORT();
				if(isFalsey(peek(0))) vm.ip += offset;
				break;
			case OP_LIMIT: ;
				uint32_t lowerBound = readInteger();
				uint32_t upperBound = readInteger();
				uint16_t limitOffset = READ_SHORT();
				if(vm.frameIndex < lowerBound || vm.frameIndex > upperBound){
					vm.ip += limitOffset;
			} break;
			case OP_GET_GLOBAL: {
				ObjString* getString = AS_STRING(READ_CONSTANT());	
				Value stored = getValue(vm.globals, getString);
				push(stored);
			} break;
			case OP_DEF_GLOBAL: {
				ObjString* setString = AS_STRING(READ_CONSTANT());	
				Value expr = pop();
				insert(vm.globals, setString, expr);
			} break;
			case OP_GET_SCOPE: {
				Value scopeVal = pop();
				Value getVal = READ_CONSTANT();
				if(IS_NULL(scopeVal) || !IS_SCOPE(scopeVal)){
					runtimeError("'%s' is undefined.", AS_CSTRING(getVal));
				}else{
					ObjScope* superScope = AS_SCOPE(scopeVal);
					ObjString* getString = AS_STRING(getVal);
					Value stored = getValue(superScope->map, getString);
					push(stored);	
				}
			} break;
			case OP_DEF_SCOPE: {
				Value setVal = READ_CONSTANT();
				Value encloseVal = READ_CONSTANT();
				Value expr = pop();
				Value scopeVal = pop();

				if(IS_NULL(scopeVal) || !IS_SCOPE(scopeVal)){
					ObjString* encloseString = AS_STRING(encloseVal);
					ObjString* setString = AS_STRING(setVal);

					ObjScope* newScope = allocateScope();
					insert(newScope->map, setString, expr);
					insert(vm.currentScope->map, encloseString, OBJ_VAL(newScope));

				}else{
					ObjScope* superScope = AS_SCOPE(scopeVal);
					ObjString* setString = AS_STRING(setVal);
					insert(superScope->map, setString, expr);
				}
			} break;
			case OP_LOAD_SCOPE: {
				Value closeVal = OBJ_VAL(vm.currentScope);
				push(closeVal);
			} break;	
			case OP_GET_LOCAL: ;
				uint32_t stackIndexGet = readInteger();
				Value val = vm.stack[stackIndexGet];
				push(val);
				break;
			case OP_DEF_LOCAL: ;
				uint32_t stackIndexDef = readInteger();
				vm.stack[stackIndexDef] = peek(0);
				break;
			case OP_POP:
				pop();
				break;
			case OP_PRINT:
				printValue(O_OUT, pop());
				print(O_OUT, "\n");
				break;
			case OP_RETURN:
				return INTERPRET_OK;
			case OP_CONSTANT: ;
				Value cons = READ_CONSTANT();
				push(cons);
				break;
			case OP_NEGATE:
				if(!IS_NUM(peek(0))){
					runtimeError("Operand must be a number.");
					return INTERPRET_RUNTIME_ERROR;
				}
				push(NUM_VAL(-AS_NUM(pop())));
				break;	
			case OP_ADD: ;
				b = pop();
				a = pop();
				switch(b.type){
					case VL_NUM:
						if(IS_NUM(a)){
							push(NUM_VAL(AS_NUM(a) + AS_NUM(b)));
						}else if(IS_STRING(a)){
							int strLength = (int)((ceil(log10(AS_NUM(b)))+1)*sizeof(char));
							char str[strLength];
							sprintf(str, "%f", AS_NUM(b));
							
							int combinedLength = AS_STRING(a)->length + strLength;
							char totalStr[combinedLength];
							sprintf(totalStr, "%s%s", AS_CSTRING(a), str);

							push(OBJ_VAL(internString(totalStr, combinedLength)));
						}else{
							runtimeError("Only number types and strings can be added.");
						}
						break;
					case VL_OBJ:
						if(IS_STRING(a)){
							int combinedLength = AS_STRING(a)->length + AS_STRING(b)->length;
							char concat[combinedLength];
							sprintf(concat, "%s%s", AS_CSTRING(a), AS_CSTRING(b));

							push(OBJ_VAL(internString(concat, combinedLength)));
						}else if(IS_NUM(a)){
							int strLength = (int)((ceil(log10(AS_NUM(a)))+1)*sizeof(char));
							char str[strLength];
							sprintf(str, "%f", AS_NUM(a));
							
							int combinedLength = AS_STRING(b)->length + strLength;
							char totalStr[combinedLength];
							sprintf(totalStr, "%s%s", str, AS_CSTRING(b));

							push(OBJ_VAL(internString(totalStr, combinedLength)));
						}else{
							runtimeError("Only number types and strings can be added.");
						}
						break;
					default:
						break;
				}
				break;
			case OP_SUBTRACT:
				BINARY_OP(-, NUM_VAL, double);
				break;
			case OP_DIVIDE:
				BINARY_OP(/, NUM_VAL, double);
				break;
			case OP_MULTIPLY:
				BINARY_OP(*, NUM_VAL, double);
				break;
			case OP_MODULO:
				BINARY_OP(%, NUM_VAL, int);
				break;
			case OP_EQUALS: ;
				b = pop();
				a = pop();
				push(BOOL_VAL(valuesEqual(a, b)));
				break;
			case OP_LESS:
				BINARY_OP(<, BOOL_VAL, double);
				break;
			case OP_GREATER:
				BINARY_OP(>, BOOL_VAL, double);
				break;
			case OP_TRUE:
				push(BOOL_VAL(true));
				break;
			case OP_FALSE:
				push(BOOL_VAL(false));
				break;
			case OP_NULL:
				push(NULL_VAL());
				break;
			case OP_NOT:
				push(BOOL_VAL(isFalsey(pop())));
				break;
			case OP_PI:
				push(NUM_VAL(PI));
				break;
			case OP_TAU:
				push(NUM_VAL(2*PI));
				break;
			case OP_E:
				push(NUM_VAL(E));
				break;
			case OP_T:
				push(NUM_VAL(vm.frameIndex));
				break;
		}	
	}
#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef BINARY_OP
}

static InterpretResult executeChunk(Chunk* chunk){
	Chunk* previousChunk = vm.chunk;
	uint8_t* prevIp = vm.ip;

	vm.chunk = chunk;
	vm.ip = chunk->code;

	InterpretResult chunkResult = run();	

	vm.chunk = previousChunk;
	vm.ip = prevIp;
	return chunkResult;
}


void runCompiler(CompilePackage* package, char* source);
void freeCompilationPackage(CompilePackage* code);
CompilePackage* initCompilationPackage();

InterpretResult executeCompiled(CompilePackage* code, int index){
	InterpretResult result;
	if(index < 0){
		initVM(code, index);
		result = run();
		renderFrame(vm.currentFrame);
		freeVM();
	}else{
		for(int i = code->lowerLimit; i<=code->upperLimit; ++i){
			initVM(code, i);
			result = run();
			renderFrame(vm.currentFrame);
			freeVM();
		}
	}
	return result;
}

InterpretResult interpretCompiled(CompilePackage* code, int index){
	InterpretResult result = code->result;
	if(result != INTERPRET_COMPILE_ERROR) {
		executeCompiled(code, index);
	}
	return result;
}

void runCompiler(CompilePackage* package, char* source){
	vm.runtimeObjects = NULL;
	
	if(!compile(source, package)){
		package->result = INTERPRET_COMPILE_ERROR;
	}
	#ifdef EM_MAIN
		setMaxFrameIndex(package->upperLimit);
	#endif
}