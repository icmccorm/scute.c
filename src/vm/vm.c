#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include "common.h"
#include "vm.h"
#include "value.h"
#include "debug.h"
#include "compiler.h"
#include "object.h"
#include "hashmap.h"
#include "output.h"
#include "svg.h"

VM vm;

static void resetStack(){
	vm.stackTop = vm.stack;
}

void initVM() {
	resetStack();		
	vm.objects = NULL;	
	initMap(&vm.strings);
	initMap(&vm.globals);
}

static void freeObjects(){
	
	Obj* list = vm.objects;

	while(list != NULL){
		Obj* next = list->next;
		freeObject(list);
		list = next;
	}
}

void freeVM() {
	freeObjects();
	freeMap(&vm.strings);
}

void push(Value value) {
	*vm.stackTop = value;
	vm.stackTop++;
}

Value pop(){
	vm.stackTop--;
	return *vm.stackTop;
}

static Value peek(int distance){
	return vm.stackTop[-1 - distance];
}

static void runtimeError(char* format, ...){
	va_list args;
	va_start(args, format);
	vprint(O_ERR, format, args);
	fputc('\n', stderr);
	va_end(args);

	size_t opIndex = vm.ip - vm.chunk->code;
	int line = getLine(vm.chunk, opIndex);
	print(O_ERR, "[line %d] in script\n", line);

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

static InterpretResult run() {

#ifdef DEBUG_TRACE_EXECUTION
	for(Value* slot = vm.stack; slot < vm.stackTop; slot++){
		print(O_DEBUG, "[ ");
		printValue(O_DEBUG, *slot);
		print(O_DEBUG, " ]");
	}
	print(O_DEBUG, "\n");
	printInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_CONSTANT_LONG() ()

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
			case OP_GET_GLOBAL: ;
				ObjString* setString = AS_STRING(pop());
				Value stored = getValue(&vm.globals, setString);
				push(stored);
				break;
			case OP_DEF_GLOBAL: ;
				ObjString* getString = AS_STRING(pop());
				Value expr = pop();
				insert(&vm.globals, getString, expr);
				break;
			case OP_POP:
				pop();
				break;
			case OP_PRINT:
				printValue(O_OUT, pop());
				#ifndef EM_MAIN
					print(O_OUT, "\n");
				#endif
				break;
			case OP_RETURN:
				return INTERPRET_OK;
			case OP_CONSTANT: ;
				Value test = READ_CONSTANT();
				push(test);
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

							push(OBJ_VAL(copyString(totalStr, combinedLength)));
						}else{
							runtimeError("Only number types and strings can be added.");
						}
						break;
					case VL_OBJ:
						if(IS_STRING(a)){
							int combinedLength = AS_STRING(a)->length + AS_STRING(b)->length;
							char concat[combinedLength];
							sprintf(concat, "%s%s", AS_CSTRING(a), AS_CSTRING(b));

							push(OBJ_VAL(copyString(concat, combinedLength)));
						}else if(IS_NUM(a)){
							int strLength = (int)((ceil(log10(AS_NUM(a)))+1)*sizeof(char));
							char str[strLength];
							sprintf(str, "%f", AS_NUM(a));
							
							int combinedLength = AS_STRING(b)->length + strLength;
							char totalStr[combinedLength];
							sprintf(totalStr, "%s%s", str, AS_CSTRING(b));

							push(OBJ_VAL(copyString(totalStr, combinedLength)));
						}else{
							runtimeError("Only number types and strings can be added.");
						}
						break;
				}
				break;
			case OP_RECT: ;
				Value x = pop();
				Value y = pop();
				Value w = pop();
				Value h = pop();

				ObjShape* newRect = createRect();
				initRect(newRect->shape, x, y, w, h);
				push(OBJ_VAL(newRect));
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
		}	
	}
#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

InterpretResult interpret(char* source){
	Chunk chunk;
	initChunk(&chunk);

	if(!compile(source, &chunk)){
		freeChunk(&chunk);
		return INTERPRET_COMPILE_ERROR;
	}

	vm.chunk = &chunk;
	vm.ip = vm.chunk->code;

	InterpretResult result = run();

	freeChunk(&chunk);
	return result;
}
