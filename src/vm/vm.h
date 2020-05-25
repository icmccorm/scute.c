#ifndef scute_vm_h
#define scute_vm_h

#include "chunk.h"
#include "value.h"
#include "package.h"

#define PI 3.141593
#define E 2.71828

#define STACK_MAX 256
#define INST_STACK_MAX 16

void runtimeError(char* format, ...);

struct sStackFrame {
	Value* stackOffset;
	struct sStackFrame* nextLower;
	uint8_t* returnTo;
	ObjChunk* chunkObj;
};

typedef struct sStackFrame StackFrame;

typedef struct {
	Chunk* chunk;
	uint8_t* ip;

	Value stack[STACK_MAX];
	Value* stackTop;
	Value* stackBottom;
	int stackSize;
	
	StackFrame stackFrames[STACK_MAX];
	int stackFrameCount;

	ObjShape** shapeStack;
	int shapeCount;
	int shapeCapacity;

	ObjInstance* instanceStack[INST_STACK_MAX];
	int instanceCount;

	HashMap* globals;

	Obj* runtimeObjects;
	
	int frameIndex;
} VM;

extern VM vm;

void push(Value value);
Value pop();

InterpretResult interpretCompiled(CompilePackage* code, int index);
void runCompiler(CompilePackage* package, char* source);
ObjInstance* currentInstance();

#endif
