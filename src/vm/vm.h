#ifndef scute_vm_h
#define scute_vm_h

#include "chunk.h"
#include "value.h"
#include "package.h"

#define STACK_MAX 256
#define PI ((double)3.141592653589793238462)
#define E  ((double)2.718281828459045235360)

void runtimeError(char* format, ...);

struct sStackFrame {
	Value* stackOffset;
	struct sStackFrame* nextLower;
	uint8_t* returnTo;
	ObjChunk* chunkObj;
	ObjInstance* instanceObj;
};

typedef struct sStackFrame StackFrame;

typedef struct {
	Chunk* chunk;
	uint8_t* ip;
	Value stack[STACK_MAX];
	StackFrame stackFrames[STACK_MAX];
	int stackSize;
	int stackFrameCount;
	Value* stackTop;
	Value* stackBottom;

	HashMap* globals;

	Obj* runtimeObjects;
	
	int shapeCount;
	int shapeCapacity;
	ObjShape** shapeStack;
	int frameIndex;
	
	ObjInstance* currentScope;
} VM;

extern VM vm;

void push(Value value);
Value pop();

InterpretResult interpretCompiled(CompilePackage* code, int index);
void runCompiler(CompilePackage* package, char* source);
ObjInstance* currentInstance();

#endif
