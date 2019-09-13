#include <stdio.h>
#include <string.h>

#include "common.h"
#include "object.h"
#include "value.h"
#include "memory.h"
#include "vm.h"

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
	}
}

ObjString* allocateString(char* chars, int length){
	ObjString* obj = ALLOCATE_OBJ(ObjString, OBJ_STRING);
	obj->chars = chars;
	obj->length = length;
}

ObjString* copyString(char* chars, int length){
	char* heapChars = ALLOCATE(char, length + 1);
	memcpy(heapChars, chars, length);
	heapChars[length] = '\0';

	return allocateString(heapChars, length);
}

