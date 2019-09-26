#ifndef scute_obj_link_h
#define scute_obj_link_h

#include "obj_def.h"
#include "svg.h"

typedef struct sObj Obj;
typedef struct sObjString ObjString;
typedef struct sObjShape ObjShape;
typedef struct sObjClosure ObjClosure;
typedef struct sObjShape ObjShape;

bool isObjectType(Value value, OBJType type);
ObjString* internString(char * start, int length);
ObjShape* createRect();
void freeObject(Obj* obj);

#define IS_STRING(value) (isObjectType(value, OBJ_STRING))
#define IS_SHAPE(value) (isObjectType(value, OBJ_SHAPE))
#define IS_CLOSURE(value) (isObjectType(value, OBJ_CLOSURE))

#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)

#define AS_SHAPE(value) ((ObjShape*)AS_OBJ(value))

#define ALLOCATE_OBJ(type, objType) \
	(type*) allocateObject(sizeof(type), objType)
	
#endif