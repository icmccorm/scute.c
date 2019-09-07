#include <stdio.h>
#include <stdarg.h>
#include "common.h"
#include "vm.h"
#include "value.h"
#include "debug.h"
#include "compiler.h"

VM vm;

static void resetStack(){
	vm.stackTop = vm.stack;
}

void initVM() {
	resetStack();		
}


void freeVM() {

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

static void runtimeError(const char* format, ...){
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	fputc('\n', stderr);
	va_end(args);

	size_t opIndex = vm.ip - vm.chunk->code;
	int line = getLine(vm.chunk, opIndex);
	fprintf(stderr, "[line %d] in script\n", line);

	resetStack();
}

static void binary(){

}

static InterpretResult run() {

#ifdef DEBUG_TRACE_EXECUTION
	for(Value* slot = vm.stack; slot < vm.stackTop; slot++){
		printf("[ ");
		printValue(*slot);
		printf(" ]");
	}
	printf("\n");
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
		switch(READ_BYTE()){
			case OP_RETURN:
				printValue(pop());
				printf("\n");
				return INTERPRET_OK;
			case OP_CONSTANT:;
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
			case OP_ADD:
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
			case OP_TRUE:
				push(BOOL_VAL(true));
				break;
			case OP_FALSE:
				push(BOOL_VAL(false));
				break;
			case OP_NULL:
				push(NULL_VAL);
				break;
		}	
	}
#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

InterpretResult interpret(const char* source){
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
