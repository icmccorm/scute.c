#ifndef scute_obj_link_h
#define scute_obj_link_h

#include "svg.h"
#include "obj_def.h"
#include "scanner.h"
#include "natives.h"

extern size_t objectBytesAllocated;

Obj* allocateObject(size_t size, OBJType type);
ObjChunk* allocateChunkObject(ObjString* funcName);
ObjInstance* allocateInstance(ObjInstance* super, InstanceType type, TKType subtype);
ObjNative* allocateNative(void* func);
ObjArray* allocateArray();
ObjArray* allocateArrayWithCapacity(int capacity);
ObjShape* allocateShape(ObjInstance* super, TKType shapeType);

bool isObjectType(Value value, OBJType type);
ObjString* tokenString(char * start, int length);
ObjString* string(char* start);

void freeObject(Obj* obj);

void addSegment(ObjShape* shape, ObjInstance* segment);
void addTransform(ObjShape* shape, ObjInstance* transform);

#define IS_STRING(value) (isObjectType(value, OBJ_STRING))
#define IS_INST(value) (isObjectType(value, OBJ_INST))
#define IS_CHUNK(value) (isObjectType(value, OBJ_CHUNK))
#define IS_COLOR(value) (isObjectType(value, OBJ_COLOR))
#define IS_ARRAY(value) (isObjectType(value, OBJ_ARRAY))

#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)
#define AS_INST(value) ((ObjInstance*)AS_OBJ(value))
#define AS_CHUNK(value) ((ObjChunk*)AS_OBJ(value))
#define AS_NATIVE(value) ((ObjNative*)AS_OBJ(value))
#define AS_COLOR(value) ((ObjColor*)AS_OBJ(value))
#define AS_ARRAY(value) ((ObjArray*)AS_OBJ(value))

#define AS_SHAPE(value) ((ObjShape*) AS_INST(value))

#define ALLOCATE_OBJ(type, objType) \
	(type*) allocateObject(sizeof(type), objType)

#endif