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
	code->result = INTERPRET_OK;

	code->animations = NULL;
	code->numAnimations = 0;
	code->animCapacity = 0;

	code->objects = NULL;
	heap = &code->objects;

	code->compiled = NULL;
	initMap(&code->strings);
	initMap(&code->globals);
	return code;
}


void addAnimation(CompilePackage* package, ObjAnim* anim){
	if(package->numAnimations + 1 > package->animCapacity){
		int oldCapacity = package->animCapacity;
		package->animCapacity = GROW_CAPACITY(oldCapacity);
		package->animations = GROW_ARRAY(package->animations, ObjAnim*, oldCapacity, package->animCapacity);
	}
	package->animations[package->numAnimations] = anim;
	++package->numAnimations;
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
	
	heap = NULL;
	freeObjects(code->objects);

	FREE_ARRAY(ObjAnim*, code->animations, code->animCapacity);
	FREE(CompilePackage, code);
}