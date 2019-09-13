#include <stdio.h>
#include "memory.h"
#include "value.h"
#include "object.h"

void initValueArray(ValueArray* array){
	array-> values = NULL;
	array-> capacity = 0;
	array-> count = 0;
}

void writeValueArray(ValueArray* array, Value value){
	if(array->capacity < array->count + 1){
		int oldCapacity = array->capacity;
		array->capacity = GROW_CAPACITY(oldCapacity);
		array->values = GROW_ARRAY(array->values, Value,
			        oldCapacity, array->capacity);
	}

	array->values[array->count] = value;
	array->count++;

}

void freeValueArray(ValueArray* array){
	FREE_ARRAY(Value, array->values, array->capacity);
	initValueArray(array);

}

void printObject(Value value);

void printValue(Value value){
	switch(value.type){
		case VL_NULL:
			printf("NULL");
			break;
		case VL_NUM:
			printf("%g", AS_NUM(value));
			break;
		case VL_BOOL:
			printf("%s", AS_BOOL(value) ? "true" : "false");
			break;
		case VL_OBJ:
			printObject(value);
			break;
		default:
			printf("%g", AS_NUM(value));
			break;
	}
}

void printObject(Value value){
	switch(OBJ_TYPE(value)){
		case OBJ_STRING:
			printf("%s", AS_CSTRING(value));
			break;
		default:
			break;
	}
}