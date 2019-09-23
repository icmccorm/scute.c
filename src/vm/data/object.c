#include <stdio.h>
#include <string.h>
#include "common.h"
#include "object.h"
#include "value.h"
#include "memory.h"
#include "vm.h"
#include "hashmap.h"
#include "svg.h"

bool isObjectType(Value value, OBJType type){
	return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

Obj* allocateObject(size_t size, OBJType type){
	Obj* obj = (Obj*) reallocate(NULL, 0, size);
	obj->type = type;
	obj->next = vm.objects;
	vm.objects = obj;
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
			FREE(ObjShape, svg);
	}
}

ObjString* allocateString(char* chars, int length){
	ObjString* obj = ALLOCATE_OBJ(ObjString, OBJ_STRING);
	obj->chars = chars;
	obj->length = length;
	obj->hash = hashFunction(chars, length);

	insert(&vm.strings, obj, NULL_VAL());
	return obj;
}

ObjShape* createRect(Value x, Value y, Value w, Value h){
	Rect* rect = ALLOCATE(Rect, 1);
	rect->shape.type = SP_RECT;
	rect->x = AS_NUM(x);
	rect->y = AS_NUM(y);
	rect->height = AS_NUM(h);
	rect->width = AS_NUM(w);
	
	ObjShape* shapeObj = ALLOCATE_OBJ(ObjShape, OBJ_SHAPE);
	shapeObj->shape = (Shape*) rect;
	return shapeObj;
}

ObjString* copyString(char* chars, int length){

	ObjString* interned = findKey(&vm.strings, chars, length);
	if(interned != NULL) return interned;
	
	char* heapChars = ALLOCATE(char, length + 1);
	memcpy(heapChars, chars, length);
	heapChars[length] = '\0';

	return allocateString(heapChars, length);
}

