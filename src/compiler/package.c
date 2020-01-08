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
	code->compiled = allocateChunkObject(NULL, CK_MAIN, TK_NULL);
	code->lowerLimit = 0;
	code->upperLimit = 0;
	code->objects = NULL;
	initMap(&code->strings);
	return code;
}

void freeObjects(Obj* list){
	while(list != NULL){
		Obj* next = list->next;
		freeObject(list);
		list = next;
	}
}

void freeCompilationPackage(CompilePackage* code){
	freeObjects(code->objects);
	freeMap(code->strings);
	FREE(CompilePackage, code);
}