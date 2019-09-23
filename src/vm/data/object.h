#ifndef scute_object_h
#define scute_object_h

#include "common.h"
#include "value.h"
#include "svg.h"

typedef enum {
	OBJ_STRING,
	OBJ_SHAPE
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

struct sObjShape{
	Obj object;
	Shape* shape;
};

bool isObjectType(Value value, OBJType type);
ObjString* copyString(char * start, int length);
ObjShape* createRect(Value x, Value y, Value w, Value h);
void freeObject(Obj* obj);

#define IS_STRING(value) (isObjectType(value, OBJ_STRING))
#define IS_SHAPE(value) (isObjectType(value, OBJ_SHAPE))

#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)

#define AS_SHAPE(value) ((ObjShape*)AS_OBJ(value))

#define ALLOCATE_OBJ(type, objType) \
	(type*) allocateObject(sizeof(type), objType)
#endif