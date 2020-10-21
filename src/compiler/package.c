#include <stdio.h>
#include <stdlib.h>
#include "package.h"
#include "chunk.h"
#include "hashmap.h"
#include "value.h"
#include "memory.h"
#include "obj.h"
#include "common.h"

CompilePackage* initCompilationPackage(){
	CompilePackage* code = ALLOCATE(CompilePackage, 1);

	code->lowerLimit = 0;
	code->upperLimit = 0;
	code->objects = NULL;
	code->compiled = NULL;
	initMap(&code->strings);
	initMap(&code->globals);
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
	freeMap(code->strings);
	freeMap(code->globals);
	freeObjects(heap);
	FREE(CompilePackage, code);
}