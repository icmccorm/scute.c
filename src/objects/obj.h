#ifndef scute_obj_link_h
#define scute_obj_link_h

#include "svg.h"
#include "obj_def.h"
#include "scanner.h"

Obj* allocateObject(size_t size, OBJType type);
ObjChunk* allocateChunkObject(ObjString* funcName, CKType chunkType, TKType instanceType);

ObjInstance* allocateInstance(ObjInstance* super);

bool isObjectType(Value value, OBJType type);
ObjString* internString(char * start, int length);
void freeObject(Obj* obj);

#define IS_STRING(value) (isObjectType(value, OBJ_STRING))
#define IS_INST(value) (isObjectType(value, OBJ_INST))
#define IS_CHUNK(value) (isObjectType(value, OBJ_CHUNK))

#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)
#define AS_INST(value) ((ObjInstance*)AS_OBJ(value))
#define AS_CHUNK(value) ((ObjChunk*)AS_OBJ(value))

#define ALLOCATE_OBJ(type, objType) \
	(type*) allocateObject(sizeof(type), objType)
	
#endif