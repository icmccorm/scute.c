#ifndef scute_closure_h
#define scute_closure_h

#include "common.h"
#include "hashmap.h"

typedef struct {
	HashMap map;
} Closure;

void set(ObjString* id, Value val);
Value get(ObjString* id);

#endif