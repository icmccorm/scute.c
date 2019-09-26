#include <stdio.h>
#include "closure.h"
#include "obj.h"
#include "value.h"
#include "memory.h"
#include "hashmap.h"

void init(Closure* close) {
	initMap(&close->map);
}

void set(ObjString* id, Value val){
	
}

void extend(Closure* close){
	
}
