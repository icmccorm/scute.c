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

typedef enum {
	OBJ_STRING,
	OBJ_SHAPE,
	OBJ_CLOSURE,
	OBJ_INSTANCE,
	OBJ_CHUNK,
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

struct sObjClosure{
	Obj object;
	HashMap* map;
	Shape* shape;
};

struct sObjChunk{
	Obj object;
	Chunk* chunk;
};

struct sShape {
	TKType shapeType;
	Shape* next;
};

struct sRect {
	Shape shape;
	Value x;
	Value y;
	Value w;
	Value h;
};
#endif