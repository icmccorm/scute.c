#ifndef scute_package_h
#define scute_package_h
#include "common.h"
#include "chunk.h"
#include "hashmap.h"
#include "value.h"

typedef struct {
	InterpretResult result;
	Obj* objects;
	ObjChunk* compiled;
	int lowerLimit;
	int upperLimit;
	HashMap* strings;
	HashMap* globals;
} CompilePackage;

void freeCompilationPackage(CompilePackage* code);
CompilePackage* initCompilationPackage();
void freeObjects(Obj* list);

#endif

