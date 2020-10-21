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
	extern void em_setMaxFrameIndex(int index);
	extern void em_addStage(Value* value, uint8_t* op);
#endif

VM vm;

static void resetStack(){
	vm.stackTop = vm.stack;
}

static bool pushStackFrame(ObjClosure* closure, ObjInstance* super, uint8_t numParams);
static uint8_t* popStackFrame();
void initGlobals(HashMap* map);
Value executeThunk(ObjClosure* thunk, int index);

void initVM(CompilePackage* package, int frameIndex) {
	package->objects = heap;
	heap = NULL;

	vm.frameIndex = frameIndex;
	vm.stackSize = 0;
	vm.stackFrameCount = 0;

	vm.shapeCapacity = 0;
	vm.shapeCount = 0;
	vm.shapeStack = NULL;
	vm.ip = NULL;	

	vm.openUpvalues = NULL;
	vm.instanceCount = 0;
	vm.package = package;
	resetStack();
	initMap(&vm.globals);
	initGlobals(vm.globals);
  	mergeMaps(package->globals, vm.globals);
}

void freeVM() {
	freeMap(vm.globals);
	FREE_ARRAY(ObjShape*, vm.shapeStack, vm.shapeCapacity);
	freeObjects(heap);
	vm.chunk = NULL;
	heap = vm.package->objects;
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
	return vm.instanceStack[vm.instanceCount-1];
}

static Chunk* currentChunk(){
	return currentStackFrame()->closeObject->chunkObj->chunk;
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
	Value peeked = vm.stackTop[-1 - distance];
	return peeked;
}

InterpretResult runtimeError(char* format, ...){
	Chunk* currentChunk = currentStackFrame()->closeObject->chunkObj->chunk;
	size_t opIndex = vm.ip - currentChunk->code;
	int line = getLine(currentChunk, opIndex);
	print(O_ERR, "[line %d] ", line);
	
	va_list args;
	va_start(args, format);
	vprint(O_ERR, format, args);
	print(O_ERR, "\n");
	va_end(args);
	resetStack();
	return INTERPRET_RUNTIME_ERROR;
}

static void pushInstance(ObjInstance* instance){
	vm.instanceStack[vm.instanceCount] = instance;
	++vm.instanceCount;
}

static bool pushStackFrame(ObjClosure* closure, ObjInstance* super, uint8_t numParams){
	if(vm.stackFrameCount < STACK_MAX - 1){
		
		StackFrame* newFrame = &(vm.stackFrames[vm.stackFrameCount]);
		
		newFrame->closeObject = closure;

		++vm.stackFrameCount;

		newFrame->stackOffset = vm.stackTop - numParams;
		newFrame->returnTo = vm.ip;
		
		vm.ip = newFrame->closeObject->chunkObj->chunk->code;

		return true;
	}
	return false;
}

static uint8_t* popStackFrame(){
	StackFrame* frame = currentStackFrame();
	--vm.stackFrameCount;
	uint8_t* opLocation = vm.stackFrames[vm.stackFrameCount].returnTo;
	return opLocation;
}

static void popInstance(bool pushback){
	if(pushback){
		push(OBJ_VAL(currentInstance()));
	}
	--vm.instanceCount;
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

static InterpretResult callFunction(ObjClosure* closeObject);

void transferIndex(Value* a, Value* b, Value* result){
	#define TRACED(value) (value->lineIndex > -1)

	bool aTraced = TRACED(a);
	bool bTraced = TRACED(b);

	if(aTraced && bTraced){
		result->lineIndex = b->lineIndex;
		result->inlineIndex = b->inlineIndex;
	}else{
		if(aTraced){
			result->lineIndex = a->lineIndex;
			result->inlineIndex = a->inlineIndex;
		}else{
			result->lineIndex = b->lineIndex;
			result->inlineIndex = b->inlineIndex;
		}
	}
	#undef TRACED
}

static ObjUpvalue* captureUpvalue(Value* local){
	ObjUpvalue* prevUpvalue = NULL;
	ObjUpvalue* upvalue = vm.openUpvalues;

	while(upvalue != NULL && upvalue->location > local){
		prevUpvalue = upvalue;
		upvalue = upvalue->next;
	}

	if(upvalue != NULL && upvalue->location == local){
		return upvalue;
	}else{
		ObjUpvalue* createdUpvalue = allocateUpvalue(local);
		createdUpvalue->next = upvalue;

		if(prevUpvalue != NULL){
			prevUpvalue->next = createdUpvalue;
		}else{
			vm.openUpvalues = createdUpvalue;
		}

		return createdUpvalue;
	}	
}

void closeUpvalues(Value* stackTop){
	while(vm.openUpvalues != NULL &&
		  vm.openUpvalues->location >= stackTop){
		
		ObjUpvalue* upvalue = vm.openUpvalues;
		upvalue->closed = *upvalue->location;
		upvalue->location = &upvalue->closed;
		vm.openUpvalues = upvalue->next;
	}
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
			return runtimeError("Operands must be numbers"); \
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
			case OP_CLOSE_UPVALUE:{
				closeUpvalues(vm.stackTop - 1);
				pop();
			} break;
			case OP_CLOSURE:{
				ObjChunk* chunk = AS_CHUNK(READ_CONSTANT());
				ObjClosure* close = allocateClosure(chunk);
				push(OBJ_VAL(close));
				StackFrame* frame = currentStackFrame();
				for(int i = 0; i<close->upvalueCount; ++i){
					uint8_t isLocal = READ_BYTE();
					uint8_t index = READ_INT();
					if(isLocal){
						close->upvalues[i] = captureUpvalue(frame->stackOffset + index);
					}else{
						close->upvalues[i] = currentStackFrame()->closeObject->upvalues[index];
					}
				}
			} break;
			case OP_GET_UPVALUE: {
				uint32_t stackIndex = readInteger();
				StackFrame* frame = currentStackFrame();
				push(*(frame->closeObject->upvalues[stackIndex]->location));
			} break;
			case OP_DEF_UPVALUE: {
				uint32_t stackIndex = readInteger();
				StackFrame* frame = currentStackFrame();
				*frame->closeObject->upvalues[stackIndex]->location = peek(0);
			} break;
			case OP_INIT_INST:{
				pushInstance(allocateInstance(NULL));
			}break;
			case OP_POP_INST:{
				bool pushBack = (bool) READ_BYTE();
				popInstance(pushBack);
			 }break;
			case OP_PUSH_INST: {
				Value isInstance = pop();
				if(IS_INST(isInstance)){
					pushInstance(AS_INST(isInstance));
				}else{
					return runtimeError("Only instances can be dereferenced.");
				}	
			} break;
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
					return runtimeError("Property does not exist.");
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
						return runtimeError("Cannot dereference NULL.");
					}else{
						return runtimeError("Can only dereference objects.");
					}
				}
				} break;
			case OP_JMP: {
				signed short jumpDistance = READ_SHORT();
				vm.ip += jumpDistance;
				} break;
			case OP_RETURN: ;
				closeUpvalues(currentStackFrame()->stackOffset);
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
						case OBJ_CLOSURE: {
							ObjClosure* closeObj = (ObjClosure*) object;
							bool success = pushStackFrame(closeObj, NULL, numParams);
							if(!success) return runtimeError("Stack overflow.");

							for(int i = numParams; i< closeObj->chunkObj->numParameters; ++i){
								push(NULL_VAL());
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
			case OP_ANIM:{
				ObjInstance* inst = currentInstance();
				ObjString* property = (ObjString*) AS_OBJ(READ_CONSTANT());
				ObjAnim* anim = (ObjAnim*) AS_OBJ(READ_CONSTANT());
				ObjClosure* close = (ObjClosure*) AS_OBJ(pop());	
				uint32_t steps = READ_INT();

				uint16_t max = steps & 0xff;
				uint16_t min = (steps >> 16) & 0xff;

				Value test = executeThunk(close, 0);

				if(inst->type == INST_SHAPE || inst->type == INST_SEG){
					animateProperty(anim, property, close, min, max);
				}else{	
					runtimeError("Only instances of shapes and segments can have animated properties.");
				}
			} break;
			case OP_JMP_FALSE: ;
				signed short offset = READ_SHORT();
				Value boolVal = pop();
				if(isFalsey(boolVal)) vm.ip += (offset);
				break;
			case OP_LIMIT: {
				uint32_t lowerBound = readInteger();
				uint32_t upperBound = readInteger();
				signed short limitOffset = READ_SHORT();
				if(vm.frameIndex < lowerBound || vm.frameIndex > upperBound){
					vm.ip += limitOffset;
				}
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
				ObjInstance* inst = currentInstance();
				Value closeVal = OBJ_VAL(inst);
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
				BINARY_OP(+, NUM_VAL, double);
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
			case OP_FRAME_INDEX:
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
	initVM(code, index);
	ObjClosure* close = allocateClosure(code->compiled);
	pushStackFrame(close, NULL, 0);

	#ifndef EM_MAIN
		printMem("before runtime");
	#endif
	result = run();
	
	#ifndef EM_MAIN
		printMem("after runtime");
	#endif
	//renderFrame(code);
	freeVM();

	#ifndef EM_MAIN
		printMem("after fruntime");
	#endif
	return result;
}

InterpretResult interpretCompiled(CompilePackage* code, int index){
	InterpretResult result = code->result;
	if(result != INTERPRET_COMPILE_ERROR) {
		result = executeCompiled(code, index);
	}
	return result;
}

Value executeThunk(ObjClosure* thunk, int index){
	initVM(currentResult(), index);
	pushStackFrame(thunk, NULL, 0);

	InterpretResult thunkInterpretSuccess = run();

	Value result = *vm.stackTop;

	freeVM();
	return result;
}
void runCompiler(CompilePackage* package, char* source){	
	bool compiled = compile(source, package);
	#ifndef EM_MAIN
		printMem("after compile");
	#endif

	if(!compiled) package->result = INTERPRET_COMPILE_ERROR;

	#ifdef EM_MAIN
		em_setMaxFrameIndex(package->upperLimit);
	#endif
}