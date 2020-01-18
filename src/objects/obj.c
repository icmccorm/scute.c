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
#include "natives.h"
#include "color.h"

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
		case(OBJ_INST): ;
			ObjInstance* close = (ObjInstance*) obj;
			freeMap(close->map);
			FREE(ObjInstance, close);
			break;
		case(OBJ_CHUNK): ;
			ObjChunk* chunkObj = (ObjChunk*) obj;
			freeChunk(chunkObj->chunk);
			FREE(ObjChunk, chunkObj);
			break;
		case(OBJ_NATIVE): ;
			ObjNative* nativeObj = (ObjNative*) obj;
			FREE(ObjNative, nativeObj);
			break;
		case(OBJ_COLOR): ;
			ObjColor* colorObj = (ObjColor*) obj;
			Color* cl = colorObj->color;
			switch(cl->colorType){
				case CL_CMYK: ;
					ColorCMYK* cmyk = (ColorCMYK*) cl;
					FREE(ColorCMYK, cmyk);
					break;
				case CL_RGB: ;
					ColorRGB* rgb = (ColorRGB*) cl;
					FREE(ColorRGB, rgb);
					break;
				case CL_HSL: ;
					ColorHSL* hsl = (ColorHSL*) cl;
					FREE(ColorHSL, hsl);
					break;
			}
			FREE(ObjColor, colorObj);
			break;
		default:
			break;
	}
}

ObjColor* allocateColor(CLType type){
	ObjColor* colorObj = ALLOCATE_OBJ(ObjColor, OBJ_COLOR);
	return colorObj;
}

ObjColor* makeRGB(Value r, Value g, Value b){
	ObjColor* colorObj = allocateColor(CL_RGB);
	ColorRGB* rgb = ALLOCATE(ColorRGB, 1);
	rgb->color.colorType = CL_RGB;
	rgb->r = r;
	rgb->g = g;
	rgb->b = b;
	colorObj->color = (Color*) rgb;
	return colorObj;
}

/*
ObjColor* makeHSL(uint16_t h, float s, float l){
	
}

ObjColor* makeCMYK(float c, float m, float y, float k){

}*/

ObjString* allocateString(char* chars, int length){
	ObjString* obj = ALLOCATE_OBJ(ObjString, OBJ_STRING);
	obj->chars = chars;
	obj->length = length;
	obj->hash = hashFunction(chars, length);

	add(currentResult()->strings, obj, NULL_VAL());
	return obj;
}

ObjNative* allocateNative(void* func){
	ObjNative* obj = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
	obj->function = (NativeFn) func;
	return obj;
}

ObjInstance* allocateInstance(ObjInstance* super){
	ObjInstance* close = ALLOCATE_OBJ(ObjInstance, OBJ_INST);
	initMap(&close->map);
	if(super != NULL){
		close->instanceType = super->instanceType;

		HashEntry* current = super->map->first;
		while(current != NULL){
			add(close->map, current->key, current->value);
			current = current->next;
		}
	}else{
		close->instanceType = TK_NULL;
	}
	return close;
}

ObjChunk* allocateChunkObject(ObjString* funcName){
	ObjChunk* chunkObj = ALLOCATE_OBJ(ObjChunk, OBJ_CHUNK);

	chunkObj->chunk = ALLOCATE(Chunk, 1);
	initChunk(chunkObj->chunk);

	chunkObj->numParameters = 0;
	chunkObj->funcName = funcName;
	chunkObj->chunkType = CK_UNDEF;
	chunkObj->instanceType = TK_NULL;	
	chunkObj->superChunk = NULL;
	return chunkObj;
}

ObjString* internString(char* chars, int length){
	ObjString* interned = findKey(currentResult()->strings, chars, length);
	if(interned != NULL) return interned;
	
	char* heapChars = ALLOCATE(char, length + 1);
	memcpy(heapChars, chars, length);
	heapChars[length] = '\0';

	return allocateString(heapChars, length);
}