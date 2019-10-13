#ifndef scute_obj_link_h
#define scute_obj_link_h

#include "svg.h"
#include "obj_def.h"

Obj* allocateObject(size_t size, OBJType type);
ObjShape* allocateShape(SPType type);
bool isObjectType(Value value, OBJType type);
ObjString* internString(char * start, int length);
void freeObject(Obj* obj);

#define IS_STRING(value) (isObjectType(value, OBJ_STRING))
#define IS_SHAPE(value) (isObjectType(value, OBJ_SHAPE))
#define IS_CLOSURE(value) (isObjectType(value, OBJ_CLOSURE))

#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)

#define AS_SHAPE(value) ((ObjShape*)AS_OBJ(value))
#define RECT() ((ObjShape*)allocateShape(SP_RECT))
#define CIRC() ((ObjShape*)allocateShape(SP_CIRC))
#define SHAPE(type) ((ObjShape*)(allocateShape(type))

#define ALLOCATE_OBJ(type, objType) \
	(type*) allocateObject(sizeof(type), objType)
	
#endif