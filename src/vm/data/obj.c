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

bool isObjectType(Value value, OBJType type){
	return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

Obj* allocateObject(size_t size, OBJType type){
	Obj* obj = (Obj*) reallocate(NULL, 0, size);
	obj->type = type;
	#ifdef INTERPRETING
		obj->next = vm.runtimeObjects;
		vm.runtimeObjects = obj;
	#else
		obj->next = currentResult()->objects;
		currentResult()->objects = obj;
	#endif
	
	return obj;
}

void freeObject(Obj* obj){
	switch(obj->type){
		case(OBJ_STRING): ;
			ObjString* string = (ObjString*) obj;
			FREE_ARRAY(char, string->chars, string->length);
			FREE(ObjString, string);
			break;
		case(OBJ_SHAPE): ;
			ObjShape* svg = (ObjShape*) obj;
			freeMap(svg->defs);
			FREE(ObjShape, svg);
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

ObjShape* allocateShape(SPType type){
	ObjShape* obj = ALLOCATE_OBJ(ObjShape, OBJ_SHAPE);
	obj->type = OBJ_SHAPE;
	initMap(&obj->closure.map);
	initMap(&obj->defs);
	initShape(type, obj->closure.map);
	return obj;
}

ObjString* internString(char* chars, int length){
	ObjString* interned = findKey(currentResult()->strings, chars, length);
	if(interned != NULL) return interned;
	
	char* heapChars = ALLOCATE(char, length + 1);
	memcpy(heapChars, chars, length);
	heapChars[length] = '\0';

	return allocateString(heapChars, length);
}

