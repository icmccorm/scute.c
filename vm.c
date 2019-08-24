#include <stdio.h>
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

#define BINARY_OP(op, typeName) \
	do { \
		typeName b = pop(); \
		typeName a = pop(); \
		push(a op b); \
	} while(false)

	for(;;) {
		uint8_t instruction;
		switch(instruction = READ_BYTE()){
			case OP_RETURN:
				printValue(pop());
				printf("\n");
				return INTERPRET_OK;
			case OP_CONSTANT:{
				Value test = READ_CONSTANT();
				push(test);
				break;
			}
			case OP_NEGATE:
				push(-pop());
				break;	
			case OP_ADD:		BINARY_OP(+, double);
			case OP_SUBTRACT:	BINARY_OP(-, double);
			case OP_MULTIPLY:	BINARY_OP(*, double);
			case OP_DIVIDE:		BINARY_OP(/, double);
			case OP_MODULO:		BINARY_OP(%, int);
		}	
	}
#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP

}

InterpretResult interpret(const char* source){
	compile(source);
	return INTERPRET_OK;
}
