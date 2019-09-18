#ifndef scute_vm_h
#define scute_vm_h

#include "chunk.h"
#include "value.h"
#include "object.h"

#define STACK_MAX 256
#define PI ((double)3.141592653589793238462)
#define E  ((double)2.718281828459045235360)


typedef struct {
	Chunk* chunk;
	uint8_t* ip;
	Value stack[STACK_MAX];
	Value* stackTop;
	Obj* objects;
	HashMap strings;
} VM;

extern VM vm;

void initVM();
void freeVM();

void push(Value value);
Value pop();

typedef enum {
	INTERPRET_OK,
	INTERPRET_COMPILE_ERROR,
	INTERPRET_RUNTIME_ERROR,
} InterpretResult;

InterpretResult interpret(char* source);

#endif
