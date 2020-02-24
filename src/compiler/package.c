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
	code->links = NULL;
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
	freeObjects(code->objects);
	FREE_ARRAY(ValueLink, code->links, code->linkCapacity);
	FREE(CompilePackage, code);
}

uint32_t addLink(CompilePackage* code, int lineIndex, int inlineIndex){
	if(code->linkCount + 1 >= code->linkCapacity){
		int oldCapacity = code->linkCapacity;
		code->linkCapacity = GROW_CAPACITY(oldCapacity);
		code->links = GROW_ARRAY(code->links, ValueLink,
		oldCapacity, code->linkCapacity);
	}
	code->links[code->linkCount].lineIndex = inlineIndex;
	code->links[code->linkCount].inlineIndex = inlineIndex;
	++code->linkCount;

	return (code->linkCount - 1);
}