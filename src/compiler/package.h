#ifndef scute_package_h
#define scute_package_h
#include "common.h"
#include "chunk.h"
#include "hashmap.h"
#include "value.h"

typedef struct {
    uint32_t lineIndex;
    uint32_t inlineIndex;
} ValueLink;

typedef struct {
	InterpretResult result;
	Obj* objects;
	ObjChunk* compiled;
	int lowerLimit;
	int upperLimit;
	HashMap* strings;
	HashMap* globals;
	ValueLink* links;
	uint32_t linkCapacity;
	uint32_t linkCount;
} CompilePackage;

void freeCompilationPackage(CompilePackage* code);
CompilePackage* initCompilationPackage();
void freeObjects(Obj* list);
void initCompilingChunk(CompilePackage* package);
uint32_t addLink(CompilePackage* code, int lineIndex, int inlineIndex);

#endif

