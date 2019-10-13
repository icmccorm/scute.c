#ifndef scute_vm_h
#define scute_vm_h

#include "chunk.h"
#include "value.h"
#include "obj.h"

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
	HashMap globals;
	uint32_t frameIndex;
	int upperLimit;
    int lowerLimit;
} VM;

typedef enum {
	INTERPRET_OK,
	INTERPRET_COMPILE_ERROR,
	INTERPRET_RUNTIME_ERROR,
} InterpretResult;

typedef struct {
	Chunk* compiled;
	InterpretResult result;
} CompiledCode;

extern VM vm;

void initVM();
void freeVM();

void push(Value value);
Value pop();

InterpretResult interpret(char* source);
InterpretResult interpretCompiled(CompiledCode* code, int index);
CompiledCode* runCompiler(char* source);

#endif
