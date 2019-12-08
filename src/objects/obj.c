#include <stdio.h>
#include <string.h>

#include "common.h"
#include "obj.h"
#include "value.h"
#include "memory.h"
#include "vm.h"
#include "hashmap.h"
#include "svg.h"
#include "compiler.h"
#include "scanner.h"

bool isObjectType(Value value, OBJType type){
	return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

Obj* allocateObject(size_t size, OBJType type){
	Obj* obj = (Obj*) reallocate(NULL, 0, size);
	obj->type = type;
	if(vm.chunk){
		obj->next = vm.runtimeObjects;
		vm.runtimeObjects = obj;
	}else{
		obj->next = vm.runtimeObjects;
		vm.runtimeObjects = obj;
	}
	return obj;
}

void freeObject(Obj* obj){
	switch(obj->type){
		case(OBJ_STRING): ;
			ObjString* string = (ObjString*) obj;
			FREE_ARRAY(char, string->chars, string->length);
			FREE(ObjString, string);
			break;
		case(OBJ_CLOSURE): ;
			ObjClosure* close = (ObjClosure*) obj;
			freeMap(close->map);
			FREE(ObjClosure, close);
			break;
		case(OBJ_CHUNK): ;
			ObjChunk* chunk = (ObjChunk*) obj;
			freeChunk(chunk->chunk);
			FREE(ObjChunk, close);
		default:
			break;
	}
}

ObjString* allocateString(char* chars, int length){
	ObjString* obj = ALLOCATE_OBJ(ObjString, OBJ_STRING);
	obj->chars = chars;
	obj->length = length;
	obj->hash = hashFunction(chars, length);

	insert(currentResult()->strings, obj, NULL_VAL());
	return obj;
}

ObjChunk* allocateChunkObject(){
	ObjChunk* obj = ALLOCATE_OBJ(ObjChunk, OBJ_CHUNK);
	initChunk(obj->chunk);
	return obj;
}

ObjClosure* allocateShapeClosure(Value shapeType){
	ObjClosure* close = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
	initMap(&close->map);
	
	TKType shapeToken = (TKType) AS_NUM(shapeType);
	close->shapeType = shapeToken;
	close->next = vm.shapes;
	vm.shapes = close;
	return close;
}
ObjClosure* allocateClosure(){
	ObjClosure* close = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
	initMap(&close->map);
	close->shapeType = TK_NULL;
	return close;
}

ObjString* internString(char* chars, int length){
	ObjString* interned = findKey(currentResult()->strings, chars, length);
	if(interned != NULL) return interned;
	
	char* heapChars = ALLOCATE(char, length + 1);
	memcpy(heapChars, chars, length);
	heapChars[length] = '\0';

	return allocateString(heapChars, length);
}