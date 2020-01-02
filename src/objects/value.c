#include <stdio.h>
#include "memory.h"
#include "scanner.h"
#include "value.h"
#include "obj.h"
#include "output.h"
#include "hashmap.h"


void initValueArray(ValueArray* array){
	array-> values = NULL;
	array-> capacity = 0;
	array-> count = 0;
}

int writeValueArray(ValueArray* array, Value value){
	if(array->capacity < array->count + 1){
		int oldCapacity = array->capacity;
		array->capacity = GROW_CAPACITY(oldCapacity);
		array->values = GROW_ARRAY(array->values, Value,
			        oldCapacity, array->capacity);
	}

	array->values[array->count] = value;
	array->count++;
	return array->count - 1;
}

void freeValueArray(ValueArray* array){
	FREE_ARRAY(Value, array->values, array->capacity);
	initValueArray(array);

}

void printObject(OutType out, Value value);

void printValue(OutType out, Value value){
	switch(value.type){
		case VL_NULL:
			print(out, "NULL");
			break;
		case VL_NUM:
			print(out, "%g", AS_NUM(value));
			break;
		case VL_BOOL:
			print(out,"%s", AS_BOOL(value) ? "true" : "false");
			break;
		case VL_OBJ:
			printObject(out, value);
			break;
		default:
			print(out, "%g", AS_NUM(value));
			break;
	}
}

void printShape(OutType out, Shape* shape);

void printObject(OutType out, Value value){
	switch(OBJ_TYPE(value)){
		case OBJ_STRING:
			print(out, "%s", AS_CSTRING(value));
			break;
		case OBJ_SCOPE:
			printMap(O_OUT, AS_SCOPE(value)->map, 0);
			break;
		default:
			break;
	}
}

void printShapeType(OutType out, TKType type){
	switch(type){
		case TK_RECT:
			print(out, "rect");
			break;
		default:
			print(out, "none");
	}
}