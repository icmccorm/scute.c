#ifndef scute_object_h
#define scute_object_h

#include "common.h"
#include "value.h"

typedef enum {
	OBJ_STRING,
	OBJ_FUNC,

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
};

bool isObjectType(Value value, OBJType type);
ObjString* copyString(char * start, int length);
void freeObject(Obj* obj);

#define IS_STRING(value) (isObjectType(value, OBJ_STRING))
#define IS_FUNC(value) (isObjectType(value, OBJ_FUNC))

#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)

#define ALLOCATE_OBJ(type, objType) \
	(type*)allocateObject(sizeof(type), objType)

#endif