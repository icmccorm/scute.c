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
#include "compiler.h"

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
		obj->next = currentResult()->objects;
		currentResult()->objects = obj;
	}
	return obj;
}

void freeObject(Obj* obj){
	switch(obj->type){
		case(OBJ_STRING): ;
			ObjString* string = (ObjString*) obj;
			FREE_ARRAY(char, string->chars, string->length + 1);
			FREE(ObjString, string);
			break;
		case(OBJ_INST): ;
			ObjInstance* close = (ObjInstance*) obj;
			freeMap(close->map);
			if(close->type == INST_SHAPE){
				ObjShape* shape = (ObjShape*) close;
				FREE_ARRAY(ObjShape*, shape->segments, shape->segmentCapacity);
				FREE(ObjShape, shape);
			}else{
				FREE(ObjInstance, close);
			}
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
		case(OBJ_ARRAY): ;
			ObjArray* arrayObj = (ObjArray*) obj;
			freeValueArray(arrayObj->array);
			FREE(ObjArray, arrayObj);
			break;
		case(OBJ_CLOSURE): ;
			ObjClosure* closeObj = (ObjClosure*) obj;
			FREE(ObjClosure, closeObj);
			break;
		default:
			print(O_OUT, "Object type not found.");
			break;
	}
}

ObjShape* allocateShape(ObjInstance* super, TKType shapeType){
	ObjShape* shape = ALLOCATE_OBJ(ObjShape, OBJ_INST);
	ObjInstance* inst = (ObjInstance*) shape;
	inst->type = INST_SHAPE;

	initMap(&inst->map);
	shape->segmentCapacity = 0;
	shape->numSegments = 0;
	shape->shapeType = shapeType;
	shape->segments = NULL;

	if(super != NULL){
		HashEntry* current = super->map->first;
		while(current != NULL){
			add(inst->map, current->key, current->value);
			current = current->next;
		}
	}
	return shape;
}

void addSegment(ObjShape* shape, ObjShape* segment){
	if(shape->numSegments + 1 >= shape->segmentCapacity){
			int oldCapacity = shape->segmentCapacity;
			shape->segmentCapacity = GROW_CAPACITY(oldCapacity);
			shape->segments = GROW_ARRAY(shape->segments, ObjShape*,
			oldCapacity, shape->segmentCapacity);
	}
	shape->segments[shape->numSegments] = segment;
	++shape->numSegments;
}

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
		HashEntry* current = super->map->first;
		while(current != NULL){
			add(close->map, current->key, current->value);
			current = current->next;
		}
	}
	return close;
}

ObjChunk* allocateChunkObject(ObjString* funcName){
	ObjChunk* chunkObj = ALLOCATE_OBJ(ObjChunk, OBJ_CHUNK);

	chunkObj->chunk = ALLOCATE(Chunk, 1);
	initChunk(chunkObj->chunk);

	chunkObj->numParameters = 0;
	chunkObj->upvalueCount = 0;
	chunkObj->funcName = funcName;
	chunkObj->chunkType = CK_UNDEF;
	chunkObj->superChunk = NULL;
	return chunkObj;
}

ObjClosure* allocateClosure(ObjChunk* innerChunk){
	ObjClosure* closeObj = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
	closeObj->chunkObj = innerChunk;
	return closeObj;
}

ObjString* tokenString(char* chars, int length){
	ObjString* interned = findKey(currentResult()->strings, chars, length);
	if(interned != NULL) return interned;
	
	char* heapChars = ALLOCATE(char, length + 1);
	memcpy(heapChars, chars, length);
	heapChars[length] = '\0';

	return allocateString(heapChars, length);
}

ObjString* string(char* chars){
	int length = (int) strlen(chars);
	ObjString* interned = findKey(currentResult()->strings, chars, length);
	if(interned != NULL) return interned;
	
	char* heapChars = ALLOCATE(char, length + 1);
	memcpy(heapChars, chars, length);
	heapChars[length] = '\0';
	return allocateString(heapChars, length);
}

ObjArray* allocateArray(){
	ObjArray* objArray = ALLOCATE_OBJ(ObjArray, OBJ_ARRAY);
	objArray->array = ALLOCATE(ValueArray, 1);
	initValueArray(objArray->array);
	return objArray;
}

ObjArray* allocateArrayWithCapacity(int capacity){
	ObjArray* objArray = ALLOCATE_OBJ(ObjArray, OBJ_ARRAY);
	objArray->array = ALLOCATE(ValueArray, 1);
	initValueArrayWithCapacity(objArray->array, capacity);
	return objArray;
}