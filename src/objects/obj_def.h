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

typedef enum {
	OBJ_STRING,
	OBJ_SHAPE,
	OBJ_SCOPE,
	OBJ_INST,
	OBJ_CHUNK,
	OBJ_NATIVE,
	OBJ_COLOR,
} OBJType;

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

struct sObjInstance{
	Obj object;
	HashMap* map;
	TKType instanceType;
	ObjInstance* nextShape;
};

struct sObjChunk{
	Obj object;
	Chunk* chunk;
	ObjString* funcName;
	CKType chunkType;
	TKType instanceType;
	int numParameters;
};

struct sObjNative {
	Obj object;
	NativeFn function;
};

struct sObjColor {
	Obj object;
	Color* color;
};

#endif