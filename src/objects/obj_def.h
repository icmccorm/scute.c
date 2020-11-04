#ifndef scute_obj_def_h
#define scute_obj_def_h

#include <stdio.h>
#include <string.h>
#include "common.h"
#include "memory.h"
#include "hashmap.h"
#include "svg.h"
#include "value.h"
#include "chunk.h"
#include "natives.h"
#include "color.h"
#include "common.h"


typedef enum {
	OBJ_STRING,
	OBJ_SHAPE,
	OBJ_SCOPE,
	OBJ_INST,
	OBJ_CHUNK,
	OBJ_NATIVE,
	OBJ_COLOR,
	OBJ_ARRAY,
	OBJ_CLOSURE,
	OBJ_UPVALUE,
	OBJ_ANIM,
	OBJ_TIMELINE,
} OBJType;

typedef enum {
	INST_NONE,
	INST_SHAPE,
	INST_SEG
} InstanceType;

//definition for Obj
struct sObj {
	OBJType type;
	struct sObj* next;
};

//definition for ObjString
struct sObjString{
	Obj object;
	int length;
	char* chars;
	uint32_t hash;
};

struct sObjArray{
	Obj object;
	ValueArray* array;
};

struct sObjInstance{
	Obj object;
	HashMap* map;
	InstanceType type;
};

struct sObjChunk{
	Obj object;
	Chunk* chunk;
	ObjString* funcName;
	CKType chunkType;
	int numParameters;
	ObjChunk* superChunk;
	int upvalueCount;
};


struct sObjUpvalue {
	Obj object;
	Value* location;
	struct sObjUpvalue* next;
	Value closed;
};

struct sObjClosure{
	Obj object;
	ObjChunk* chunkObj;
	struct sObjUpvalue** upvalues;
	int upvalueCount;
};

struct sObjNative {
	Obj object;
	NativeFn function;
};

struct sObjShape {
	ObjInstance instance;
	struct sObjShape* nextShape;
	TKType shapeType;
	struct sObjShape** segments;
	struct sObjAnim* animation;
	int numSegments;
	int segmentCapacity;
};

struct sTimestep{
	Obj obj;
	int min;
	int max;
	struct sObjClosure* thunk;	
};

struct sObjTimeline{
	Obj object;
	struct sTimestep* steps;
	int numSteps;
	int stepCapacity;
	int stepIndex;
};

struct sObjAnim {
	Obj object;
	HashMap* map;
};

struct sObjShapeSegment {
	ObjInstance instance;
	TKType segmentType;
};

#endif