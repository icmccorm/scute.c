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
#include "natives.h"

#ifdef EM_MAIN
	extern void setMaxFrameIndex(int index);
	extern void em_addStage(Value* value, uint8_t* op);

#endif

VM vm;

static void resetStack(){
	vm.stackTop = vm.stack;
}

static void pushStackFrame(ObjChunk* funcChunk, ObjInstance* super, uint8_t numParams);
static uint8_t* popStackFrame();
void initGlobals(HashMap* map);


void initVM(CompilePackage* package, int frameIndex) {
	vm.frameIndex = frameIndex;
	vm.runtimeObjects = NULL;
	vm.stackSize = 0;
	vm.stackFrameCount = 0;

	vm.shapeCapacity = 0;
	vm.shapeCount = 0;
	vm.shapeStack = NULL;
	vm.ip = NULL;	

	resetStack();
	initMap(&vm.globals);
	initGlobals(vm.globals);
  	mergeMaps(package->globals, vm.globals);
	pushStackFrame(package->compiled, NULL, 0);
}

void initGlobals(HashMap* map){
	ObjString* canvasString = string("canvas");
	add(map, canvasString, OBJ_VAL(allocateInstance(NULL)));
}

void freeVM() {
	freeMap(vm.globals);
	FREE_ARRAY(ObjShape*, vm.shapeStack, vm.shapeCapacity);
	freeObjects(vm.runtimeObjects);
	vm.chunk = NULL;
}

void push(Value value) {
	*vm.stackTop = value;
	++vm.stackTop;
	++vm.stackSize;
}

static StackFrame* currentStackFrame(){
	return &vm.stackFrames[vm.stackFrameCount-1];
}	

ObjInstance* currentInstance(){
	return currentStackFrame()->instanceObj;
}

static Chunk* currentChunk(){
	return currentStackFrame()->chunkObj->chunk;
}

int stackSize(){
	return vm.stackTop - vm.stackBottom;
}

Value pop(){
	--vm.stackTop;
	--vm.stackSize;
	return *vm.stackTop;
}

static Value peek(int distance){
	return vm.stackTop[-1 - distance];
}

ObjInstance* latestInstance(){
	Value peeked = peek(0);
	if(IS_OBJ(peeked)){
		Obj* peekedObj = AS_OBJ(peeked);
		if(peeked.type == OBJ_INST){
			return (ObjInstance*) peekedObj;
		}
	}
	return currentInstance();
}

void runtimeError(char* format, ...){
	Chunk* currentChunk = currentStackFrame()->chunkObj->chunk;
	size_t opIndex = vm.ip - currentChunk->code;
	int line = getLine(currentChunk, opIndex);
	print(O_ERR, "[line %d] ", line);
	
	va_list args;
	va_start(args, format);
	vprint(O_ERR, format, args);
	print(O_ERR, "\n");
	va_end(args);
	resetStack();
}

static void pushStackFrame(ObjChunk* funcChunk, ObjInstance* super, uint8_t numParams){
	StackFrame* newFrame = &(vm.stackFrames[vm.stackFrameCount]);
	++vm.stackFrameCount;
	
	newFrame->chunkObj = funcChunk;

	if(funcChunk->chunkType == CK_CONSTR){
		if(funcChunk->instanceType != TK_NULL){
			newFrame->instanceObj = (ObjInstance*) allocateShape(super, funcChunk->instanceType);
		}else{
			newFrame->instanceObj = allocateInstance(super);
		}
	}else{
		newFrame->instanceObj = NULL;
	}

	newFrame->stackOffset = vm.stackTop - numParams;
	newFrame->returnTo = vm.ip;
	vm.ip = funcChunk->chunk->code;
}

static uint8_t* popStackFrame(){
	StackFrame* frame = currentStackFrame();
	if(frame->chunkObj->chunkType == CK_CONSTR){
		push(OBJ_VAL(frame->instanceObj));
	}
	--vm.stackFrameCount;
	uint8_t* opLocation = vm.stackFrames[vm.stackFrameCount].returnTo;
	return opLocation;
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
		default:
			return true;
			break;
	}	
}

static InterpretResult callFunction(ObjChunk* chunkObj);


void transferIndex(Value* a, Value* b, Value* result){
	Value* trace = a->lineIndex > -1 ? a : (b->lineIndex > -1 ? b : b);
	result->lineIndex = trace->lineIndex;
	result->inlineIndex = trace->inlineIndex;

	#ifdef EM_MAIN
		if(result->lineIndex >= 0) {
			em_addStage(trace, (vm.ip-1));
		}
	#endif
}

#define READ_BYTE() (*vm.ip++)
static uint32_t readInteger() {
	uint8_t numBytes = READ_BYTE();
	uint32_t index = 0; 
	for(int i = 0; i<numBytes; ++i) { 
		index = index | (READ_BYTE() << (8*i)); 
	}
	return index;
}

static Obj* valueToObject(OBJType objType, Value val){
	if(IS_OBJ(val)){
		Obj* obj = AS_OBJ(val);
		if(obj && obj->type == objType){
			return obj;
		}	
	}
	return NULL;
}

uint8_t readByte(){
	uint8_t* test = vm.ip;
	int index = vm.ip - currentChunk()->code;
	++vm.ip;
	return *(vm.ip - 1);
}

signed short readShort(){
	signed short shortVal = (signed short)(*(vm.ip) << 8 | (*(vm.ip + 1)));
	vm.ip += 2;
	return shortVal;
}

static InterpretResult run() {

#define READ_BYTE() (*vm.ip++)
#define READ_SHORT() (readShort())
#define READ_INT() (readInteger())
#define READ_CONSTANT() (currentChunk()->constants->values[readInteger()])
#define CONSTANT(index) (currentChunk()->constants->values[index])
#define BINARY_OP(op, valueType, operandType) \
	do { \
		if(!IS_NUM(peek(0)) || !IS_NUM(peek(1))){ \
			runtimeError("Operands must be numbers"); \
			return INTERPRET_RUNTIME_ERROR; \
		} \
		Value b = pop(); \
		Value a = pop(); \
		operandType typeB = AS_NUM(b); \
		operandType typeA = AS_NUM(a); \
		Value result = valueType(typeA op typeB); \
		transferIndex(&a, &b, &result); \
		push(result); \
	} while(false); \

	for(;;) {
		Value a;
		Value b;
		#ifdef DEBUG
			if(DEBUG_STACK){
				for(Value* slot = vm.stack; slot < vm.stackTop; slot++){
					print(O_DEBUG, "[ ");
					printValue(O_DEBUG, *slot);
					print(O_DEBUG, " ]");
				}
				print(O_DEBUG, "\n");
			}
		#endif
		switch(readByte()){
			case OP_BUILD_ARRAY: {
				uint32_t numElements = READ_INT();
				ObjArray* arrayObj = allocateArrayWithCapacity(numElements);
				for(int i = 1; i<= numElements; ++i){
					setValueArray(arrayObj->array, numElements - i, pop());
				}
				push(OBJ_VAL(arrayObj));
				} break;
			case OP_DEF_INST: {
				ObjString* memberId = AS_STRING(READ_CONSTANT());
				bool pushBackInstance = (bool) READ_BYTE();

				Value expression = pop();


				Value instanceVal = pop();
				if(IS_INST(instanceVal)){
					ObjInstance* inst = AS_INST(instanceVal);
					add(inst->map, memberId, expression);
				}else{
					runtimeError("Property does not exist.");
				}
				if(pushBackInstance){
					push(instanceVal);
				}else{
					push(expression);
				}
				} break;
			case OP_DEREF: {
				ObjString* memberId = AS_STRING(READ_CONSTANT());
				Value instanceVal = pop();
				if(IS_INST(instanceVal)){
					ObjInstance* inst = AS_INST(instanceVal);
					push(getValue(inst->map, memberId));
				}else{
					if(IS_NULL(instanceVal)){
						runtimeError("Cannot dereference NULL.");
					}else{
						runtimeError("Can only dereference objects.");
					}
				}
				} break;
			case OP_JMP: {
				signed short jumpDistance = READ_SHORT();
				vm.ip += jumpDistance;
				} break;
			case OP_JMP_CNT: {
				Value repeatVal = pop();
				signed short offset = READ_SHORT();
				if(IS_NUM(repeatVal)){
					int numRepeats = AS_NUM(repeatVal);
					if(numRepeats > 0){
						push(NUM_VAL(numRepeats - 1));
					}else{
						vm.ip += offset;
					}
				}else{
					switch(repeatVal.type){
						case VL_NULL:
							runtimeError("Repeat value is null.");
							break;
						default:
							runtimeError("Repeat value is not a number.");
							break;
					}
					return INTERPRET_RUNTIME_ERROR;
				}
			} break;
			case OP_RETURN: ;
				uint8_t* returnAddress = popStackFrame();
				if(returnAddress){
					vm.ip = returnAddress;
				}else{	
					return INTERPRET_OK;
				}
				break;
			case OP_CALL: {
				uint8_t numParams = READ_BYTE();

				Value fnValue = pop();
				if(fnValue.type == VL_OBJ){
					Obj* object = AS_OBJ(fnValue);
					switch(object->type){

						case OBJ_CHUNK: {
							ObjChunk* chunkObj = (ObjChunk*) object;

							Value peekVal = peek(0);
							ObjInstance* super = NULL;
							if(IS_INST(peekVal)) {
								super = AS_INST(pop());
							}

							pushStackFrame(chunkObj, NULL, numParams);

							for(int i = numParams; i< chunkObj->numParameters; ++i){
								push(NULL_VAL());
							}

							if(currentInstance()->type == INST_SHAPE){
								ObjShape* shape = (ObjShape*) currentInstance();
								shape->shapeType = chunkObj->instanceType;
							}
							} break;
						case OBJ_NATIVE: {
							ObjNative* native = (ObjNative*) object;
							NativeFn function = native->function;
							Value params[numParams];
							for(int i = 0; i<numParams; ++i){
								params[i] = pop();
							}
							Value result = function(params, numParams);
							push(result);
							} break;
						default:
							runtimeError("Only functions or constructors can be called.");
							return INTERPRET_RUNTIME_ERROR;
					}
				}else{
					runtimeError("Only functions or constructors can be called.");
					return INTERPRET_RUNTIME_ERROR;
				}
			} break;
			case OP_DRAW: {
				Value drawVal = pop();
				Obj* toObject = valueToObject(OBJ_INST, drawVal);
				if(((ObjInstance*) toObject)->type == INST_SHAPE){
					ObjShape* shape = (ObjShape*) toObject;
					pushShape(shape);
				}else{
					runtimeError("Only shapes and shape instances can be drawn.");
					return INTERPRET_RUNTIME_ERROR;
				}
			} break;
			case OP_JMP_FALSE: ;
				signed short offset = READ_SHORT();
				Value boolVal = pop();
				if(isFalsey(boolVal)) vm.ip += offset;
				break;
			case OP_LIMIT: ;
				uint32_t lowerBound = readInteger();
				uint32_t upperBound = readInteger();
				signed short limitOffset = READ_SHORT();
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
				Value expr = peek(0);
				add(vm.globals, setString, expr);
			} break;
			case OP_LOAD_INST: {
				ObjInstance* currentInstance = currentStackFrame()->instanceObj;
				Value closeVal = OBJ_VAL(currentInstance);
				push(closeVal);
			} break;	
			case OP_GET_LOCAL: {
				uint32_t stackIndex = readInteger();
				Value val = *(currentStackFrame()->stackOffset+stackIndex);
				push(val);
			} break;
			case OP_DEF_LOCAL: {
				uint32_t stackIndex = readInteger();
				*(currentStackFrame()->stackOffset+stackIndex) = peek(0);
			} break;
			case OP_POP:
				pop();
				break;
			case OP_PRINT:
				printValue(O_OUT, pop());
				print(O_OUT, "\n");
				break;
			case OP_CONSTANT: ;
				Value cons = READ_CONSTANT();
				push(cons);
				break;
			case OP_NEGATE:
				if(!IS_NUM(peek(0))){
					runtimeError("Operand must be a number.");
					return INTERPRET_RUNTIME_ERROR;
				}
				Value popped = pop();
				Value toPush = NUM_VAL(-AS_NUM(popped));
				toPush.inlineIndex = popped.inlineIndex;
				toPush.lineIndex = popped.lineIndex;
				push(toPush);
				break;	
			case OP_ADD: ;
				BINARY_OP(-, NUM_VAL, double);
				/*
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

							push(OBJ_VAL(tokenString(totalStr, combinedLength)));
						}else{
							runtimeError("Only number types and strings can be added.");
						}
						break;
					case VL_OBJ:
						if(IS_STRING(a)){
							int combinedLength = AS_STRING(a)->length + AS_STRING(b)->length;
							char concat[combinedLength];
							sprintf(concat, "%s%s", AS_CSTRING(a), AS_CSTRING(b));

							push(OBJ_VAL(tokenString(concat, combinedLength)));
						}else if(IS_NUM(a)){
							int strLength = (int)((ceil(log10(AS_NUM(a)))+1)*sizeof(char));
							char str[strLength];
							sprintf(str, "%f", AS_NUM(a));
							
							int combinedLength = AS_STRING(b)->length + strLength;
							char totalStr[combinedLength];
							sprintf(totalStr, "%s%s", str, AS_CSTRING(b));

							push(OBJ_VAL(tokenString(totalStr, combinedLength)));
						}else{
							runtimeError("Only number types and strings can be added.");
						}
						break;
					default:
						break;
						}*/
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

void runCompiler(CompilePackage* package, char* source);
void freeCompilationPackage(CompilePackage* code);
CompilePackage* initCompilationPackage();

InterpretResult executeCompiled(CompilePackage* code, int index){
	InterpretResult result;

	if(index <= 0){
		
		initVM(code, index);
		#ifndef EM_MAIN
			print(O_OUT, "[A] before runtime: %d\n\n-----\n", numBytesAllocated);
		#endif
		
		result = run();
		
		#ifndef EM_MAIN
			print(O_OUT, "-----\n\n[A] after runtime: %d\n", numBytesAllocated);
		#endif
		
		renderFrame(code);
		freeVM();

		#ifndef EM_MAIN
			print(O_OUT, "[A] after fruntime: %d\n", numBytesAllocated);
		#endif

	}else{
		for(int i = code->lowerLimit; i<=code->upperLimit; ++i){
			initVM(code, i);
			result = run();
			renderFrame(code);
			freeVM();
		}
	}
	return result;
}

InterpretResult interpretCompiled(CompilePackage* code, int index){
	InterpretResult result = code->result;

	if(result != INTERPRET_COMPILE_ERROR) {
		result = executeCompiled(code, index);
	}


	return result;
}

void runCompiler(CompilePackage* package, char* source){	
	bool compiled = compile(source, package);
	#ifndef EM_MAIN
		print(O_OUT, "[A] after compile: %d\n", numBytesAllocated);
	#endif

	if(!compiled) package->result = INTERPRET_COMPILE_ERROR;

	#ifdef EM_MAIN
		setMaxFrameIndex(package->upperLimit);
	#endif
}