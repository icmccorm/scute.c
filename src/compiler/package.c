#include <stdio.h>
#include <stdlib.h>
#include "package.h"
#include "chunk.h"
#include "hashmap.h"
#include "value.h"
#include "memory.h"
#include "obj.h"

CompilePackage* initCompilationPackage(){
	CompilePackage* code = ALLOCATE(CompilePackage, 1);
	code->compiled = ALLOCATE(Chunk, 1);
	code->lowerLimit = 0;
	code->upperLimit = 0;
	code->objects = NULL;

	initChunk(code->compiled);
	initMap(&code->strings);

	return code;
}

static void freeObjects(Obj* list){
	while(list != NULL){
		Obj* next = list->next;
		freeObject(list);
		list = next;
	}
}

void freeCompilationPackage(CompilePackage* code){
	freeObjects(code->objects);
	freeMap(&code->strings);
	FREE(Chunk, code->compiled);
	FREE(CompilePackage, code);
}

