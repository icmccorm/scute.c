#ifndef scute_obj_link_h
#define scute_obj_link_h

#include "svg.h"
#include "obj_def.h"
#include "scanner.h"

Obj* allocateObject(size_t size, OBJType type);
ObjChunk* allocateChunkObject();

ObjScope* allocateScope();
ObjScope* allocateShapeScope(Value shapeType);

bool isObjectType(Value value, OBJType type);
ObjString* internString(char * start, int length);
void freeObject(Obj* obj);

#define IS_STRING(value) (isObjectType(value, OBJ_STRING))
#define IS_SHAPE(value) (isObjectType(value, OBJ_SHAPE))
#define IS_SCOPE(value) (isObjectType(value, OBJ_SCOPE))

#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)
#define AS_SCOPE(value) ((ObjScope*)AS_OBJ(value))

#define AS_SHAPE(value) ((Shape*)(value))
#define AS_RECT(value) ((Rect*)(value))

#define ALLOCATE_OBJ(type, objType) \
	(type*) allocateObject(sizeof(type), objType)
	
#endif