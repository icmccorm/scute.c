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
//#include "natives.h"

#ifdef EM_MAIN
	extern void setMaxFrameIndex(int index);
#endif

VM vm;

static void resetStack(){
	vm.stackTop = vm.stack;
}

static void pushStackFrame(ObjChunk* funcChunk);
static uint8_t* popStackFrame();

void initVM(CompilePackage* package, int frameIndex) {
	vm.frameIndex = frameIndex;
	vm.runtimeObjects = NULL;
	vm.stackSize = 0;
	vm.stackFrameCount = 0;
	vm.currentScope = NULL;

	vm.shapeCapacity = 0;
	vm.shapeCount = 0;
	vm.shapeStack = NULL;

	vm.ip = NULL;	

	resetStack();
	initMap(&vm.globals);
  	mergeMaps(package->globals, vm.globals);
	pushStackFrame(package->compiled);
}

void freeVM() {
	freeMap(vm.globals);
	freeObjects(vm.runtimeObjects);
	FREE_ARRAY(ObjInstance*, vm.shapeStack, vm.shapeCapacity);
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

static ObjInstance* currentInstance(){
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

static void pushStackFrame(ObjChunk* funcChunk){
	StackFrame* newFrame = &(vm.stackFrames[vm.stackFrameCount]);
	++vm.stackFrameCount;
	
	newFrame->chunkObj = funcChunk;

	if(funcChunk->chunkType == CK_CONSTR){
		newFrame->instanceObj = allocateInstance(NULL);
	}else{
		newFrame->instanceObj = NULL;
	}

	newFrame->stackOffset = vm.stackTop - funcChunk->numParameters;
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

static InterpretResult run() {

#define READ_BYTE() (*vm.ip++)
#define READ_SHORT() (vm.ip += 2, (uint16_t)((vm.ip[-2] << 8) | vm.ip[-1]))
#define READ_CONSTANT() (currentChunk()->constants.values[readInteger()])
#define CONSTANT(index) (currentChunk()->constants.values[index])
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
		switch(READ_BYTE()){
			case OP_DEF_INST: {
				ObjString* memberId = AS_STRING(READ_CONSTANT());
				Value expression = pop();
				Value instanceVal = pop();
				if(IS_INST(instanceVal)){
					ObjInstance* inst = AS_INST(instanceVal);
					add(inst->map, memberId, expression);
				}else{
					runtimeError("Property does not exist.");
				}
				push(expression);
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
				int16_t offset = READ_SHORT();
				vm.ip += offset;
				} break;
			case OP_JMP_CNT: {
				Value repeatVal = pop();
				uint16_t offset = READ_SHORT();
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
							pushStackFrame(chunkObj);

							Value peekVal = peek(0);
							if(IS_INST(peekVal)) {

								ObjInstance* super = AS_INST(pop());
								ObjInstance* current = currentInstance();
								mergeMaps(super->map, current->map);
							}

							for(int i = numParams; i< chunkObj->numParameters; ++i){
								push(NULL_VAL());
							}

							currentStackFrame()->instanceObj->instanceType = chunkObj->instanceType;
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
							break;
					}
				}else{
					runtimeError("Only functions or constructors can be called.");
					return INTERPRET_RUNTIME_ERROR;
				}
			} break;
			case OP_DRAW: {
				Value drawVal = pop();
				Obj* toObject = valueToObject(OBJ_INST, drawVal);
				if(toObject){
					ObjInstance* shape = AS_INST(drawVal);
					pushShape(shape);
				}else{
					runtimeError("Only shapes and shape instances can be drawn.");
					return INTERPRET_RUNTIME_ERROR;
				}
			} break;
			case OP_JMP_FALSE: ;
				int16_t offset = READ_SHORT();
				Value boolVal = pop();
				if(isFalsey(boolVal)) vm.ip += offset;
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
				add(vm.globals, setString, expr);
			} break;
			case OP_GET_SCOPE: {
				Value scopeVal = pop();
				Value getVal = READ_CONSTANT();

				ObjInstance* superScope = (ObjInstance*) valueToObject(OBJ_INST, scopeVal);
				if(superScope){
					ObjInstance* superScope = AS_INST(scopeVal);
					ObjString* getString = AS_STRING(getVal);
					Value stored = getValue(superScope->map, getString);
					push(stored);	
				}else{
					if(IS_NULL(scopeVal)){
						runtimeError("Cannot dereference a NULL value.");
						return INTERPRET_RUNTIME_ERROR;
					}else{
						runtimeError("Only instances of classes can be dereferenced.");
						return INTERPRET_RUNTIME_ERROR;
					}
				}
			} break;
			case OP_DEF_SCOPE: {
				Value setVal = READ_CONSTANT();
				Value encloseVal = READ_CONSTANT();
				Value expr = pop();
				Value scopeVal = pop();
				if(IS_NULL(scopeVal) || !IS_INST(scopeVal)){
					ObjString* encloseString = AS_STRING(encloseVal);
					ObjString* setString = AS_STRING(setVal);
					ObjInstance* newScope = allocateInstance(NULL);
					add(newScope->map, setString, expr);
					add(vm.currentScope->map, encloseString, OBJ_VAL(newScope));

				}else{
					ObjInstance* superScope = AS_INST(scopeVal);
					ObjString* setString = AS_STRING(setVal);
					add(superScope->map, setString, expr);
				}
				push(expr);
			} break;
			case OP_LOAD_INSTANCE: {
				ObjInstance* currentInstance = currentStackFrame()->instanceObj;
				Value closeVal = OBJ_VAL(currentInstance);
				push(closeVal);
			} break;	
			case OP_GET_LOCAL: {
				uint32_t stackIndex = readInteger();
				Value val = *(vm.stackFrames->stackOffset+stackIndex);
				push(val);
			} break;
			case OP_DEF_LOCAL: {
				uint32_t stackIndex = readInteger();
				*(vm.stackFrames->stackOffset+stackIndex) = peek(0);
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
		result = run();
		renderFrame();
		freeVM();
	}else{
		for(int i = code->lowerLimit; i<=code->upperLimit; ++i){
			initVM(code, i);
			result = run();
			renderFrame();
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
	vm.runtimeObjects = NULL;
	bool compiled = compile(source, package);
	if(!compiled) package->result = INTERPRET_COMPILE_ERROR;

	#ifdef EM_MAIN
		setMaxFrameIndex(package->upperLimit);
	#endif
}