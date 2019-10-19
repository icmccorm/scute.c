#ifndef scute_obj_def_h
#define scute_obj_def_h

#include <stdio.h>
#include <string.h>

#include "common.h"
#include "memory.h"
#include "hashmap.h"
#include "svg.h"
#include "value.h"

typedef enum {
	OBJ_STRING,
	OBJ_SHAPE,
	OBJ_CLOSURE,
	OBJ_INSTANCE,
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
	bool isInstance;
	HashMap* map;
};
struct sObjShape{
	ObjClosure closure;
	HashMap* defs;
	SPType type;
};

#endif