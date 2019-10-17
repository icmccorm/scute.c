#ifndef scute_package_h
#define scute_package_h
#include "common.h"
#include "chunk.h"
#include "hashmap.h"
#include "value.h"

typedef struct {
	Chunk* compiled;
	InterpretResult result;
	Obj* objects;
	HashMap strings;
	int lowerLimit;
	int upperLimit;
} CompilePackage;

void freeCompilationPackage(CompilePackage* code);
CompilePackage* initCompilationPackage();

#endif

