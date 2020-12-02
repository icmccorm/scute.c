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

Obj** heap = NULL;

Obj* allocateObject(size_t size, OBJType type){
	Obj* obj = (Obj*) reallocate(NULL, 0, size);
	obj->type = type;
	obj->next = *heap;
	*heap = obj;
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
			FREE_ARRAY(ObjUpvalue*, closeObj->upvalues, closeObj->upvalueCount);
			FREE(ObjClosure, closeObj);
			break;
		case(OBJ_UPVALUE): ;
			ObjUpvalue* upval = (ObjUpvalue*) obj;
			FREE(ObjUpvalue, upval);
			break;
		case(OBJ_TIMELINE): ;
			ObjTimeline* timeline = (ObjTimeline*) obj;
			FREE_ARRAY(Timestep, timeline->steps, timeline->stepCapacity);
			FREE(ObjTimeline, timeline);
			break;
		case (OBJ_ANIM): ;
			ObjAnim* anim = (ObjAnim*) obj;
			freeMap(anim->map);
			FREE(ObjAnim, anim);
			break;
		default:
			print(O_OUT, "Object type not found: %d", obj->type);
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
	shape->animation = NULL;

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
	close->type = INST_NONE;
	if(super != NULL){
		HashEntry* current = super->map->first;
		while(current != NULL){
			add(close->map, current->key, current->value);
			current = current->next;
		}
	}
	return close;
}

ObjAnim* allocateAnimation(CompilePackage* package){
	heap = &currentResult()->objects;
	ObjAnim* anim = ALLOCATE_OBJ(ObjAnim, OBJ_ANIM);
	initMap(&anim->map);
	heap = &vm.objects;
	return anim;
}

void addItemToTimeline(ObjTimeline* timeline, ObjClosure* thunk, int min, int max){
	if(timeline->numSteps + 1 > timeline->stepCapacity){
        int oldCapacity = timeline->stepCapacity;
		timeline->stepCapacity = GROW_CAPACITY(oldCapacity);
		timeline->steps = GROW_ARRAY(timeline->steps, Timestep, oldCapacity, timeline->stepCapacity);
    }
	Timestep* step = &(timeline->steps[timeline->numSteps]);
	step->min = min;
	step->max = max;
	step->thunk = thunk;
	step->resolved = NULL_VAL();
	++timeline->numSteps;
}

void animateProperty(ObjAnim* anim, ObjString* propName, ObjClosure* thunk, int min, int max){
	Value propertyEntry = getValue(anim->map, propName);
	ObjTimeline* timeline = NULL;
	if(!IS_NULL(propertyEntry)){
		timeline = (ObjTimeline*) AS_OBJ(propertyEntry);
	}else{
		heap = &currentResult()->objects;
		timeline = ALLOCATE_OBJ(ObjTimeline, OBJ_TIMELINE);
		timeline->steps = NULL;
		timeline->numSteps = 0;
		timeline->stepCapacity = 0;
		timeline->stepIndex = 0;
		add(anim->map, propName, OBJ_VAL(timeline));
		heap = &vm.objects;
	}
	addItemToTimeline(timeline, thunk, min, max);
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

ObjClosure* allocateClosure(ObjChunk* innerChunk, bool saveWithCompilation){
	ObjUpvalue** upvalues = ALLOCATE(ObjUpvalue*, innerChunk->upvalueCount);
	for(int i = 0; i<innerChunk->upvalueCount; ++i){
		upvalues[i] = NULL;
	}
	if(saveWithCompilation) heap = &currentResult()->objects;

	ObjClosure* closeObj = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
	closeObj->chunkObj = innerChunk;
	closeObj->upvalueCount = innerChunk->upvalueCount;
	closeObj->upvalues = upvalues;
	
	if(saveWithCompilation) heap = &vm.objects;
	return closeObj;
}

ObjUpvalue* allocateUpvalue(Value* slot){
	ObjUpvalue* upval = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
	upval->location = slot;
	upval->next = NULL;
	upval->closed = NULL_VAL();
	return upval;
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