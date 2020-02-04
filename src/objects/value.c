#include <stdio.h>
#include "memory.h"
#include "scanner.h"
#include "value.h"
#include "obj.h"
#include "output.h"
#include "hashmap.h"
#include "chunk.h"
#include "debug.h"

void initValueArray(ValueArray* array){
	array-> values = NULL;
	array-> capacity = 0;
	array-> count = 0;
}

void initValueArrayWithCapacity(ValueArray* array, int capacity){
	array-> capacity = capacity;
	array-> count = 0;
	array-> values = NULL;
	array->capacity = GROW_CAPACITY(array->capacity);
	array->values = GROW_ARRAY(array->values, Value, 0, array->capacity);
    array->count = capacity;
}

bool shouldGrowArray(ValueArray* array){
	return array->capacity < array->count + 1;
}

void growArray(ValueArray* array){
	int oldCapacity = array->capacity;
	array->capacity = GROW_CAPACITY(oldCapacity);
	array->values = GROW_ARRAY(array->values, Value,
	oldCapacity, array->capacity);
}

int pushValueArray(ValueArray* array, Value value){
	if(shouldGrowArray(array)){
		growArray(array);
	}

	array->values[array->count] = value;
	array->count++;
	return array->count - 1;
}

void freeValueArray(ValueArray* array){
	FREE_ARRAY(Value, array->values, array->capacity);
	initValueArray(array);
}

Value getValueArray(ValueArray* array, int index){
	if(array->capacity == 0 || index < 0 || index >= array->count){
		return NULL_VAL();
	}else{
		return array->values[index];
	}
}

void printArray(OutType out, ValueArray* array){
	print(out, "[");
	if(array->count > 0){
		printValue(out, array->values[0]);
		for(int i = 1; i< array->count; ++i){
			print(out, ", ");
			printValue(out, array->values[i]);
		}
	}
	print(out, "]\n");
}

void setValueArray(ValueArray* array, int index, Value val){
	if(array->capacity > 0 && index >= 0 && index < array->capacity){
		array->values[index] = val;
	}
}

Value popValueArray(ValueArray* array){
	if(array->count > 0){
		Value val = array->values[array->count-1];
		array->count--;
		return val;
	}else{
		return NULL_VAL();
	}
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

void printObject(OutType out, Value value){
	switch(OBJ_TYPE(value)){
		case OBJ_STRING:
			print(out, "%s", AS_CSTRING(value));
			break;
		case OBJ_CHUNK:
			print(out, "<chunk>");
			print(out, "\n------------\n");
			printChunk(AS_CHUNK(value)->chunk, NULL);
			print(out, "------------\n");
			break;
		case OBJ_INST:
			printMap(O_OUT, AS_INST(value)->map, 0);
			break;
		case OBJ_COLOR: ;
			printColor(O_OUT, AS_COLOR(value)->color);
			break;
		case OBJ_ARRAY: ;
			printArray(O_OUT, AS_ARRAY(value)->array);
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
