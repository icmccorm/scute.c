#ifndef scute_value_h
#define scute_value_h

#include "common.h"
#include "output.h"
#include "obj.h"

typedef enum {
	VL_BOOL,
	VL_NULL,
	VL_NUM,
	VL_OBJ
} VLType;

typedef struct {
	VLType type;
	union {
		bool boolean;
		double number;
		Obj* obj;
	} as;
} Value;

typedef struct {
	int capacity;
	int count;
	Value * values;
} ValueArray;	

#define BOOL_VAL(value) ((Value){VL_BOOL, {.boolean = value}})
#define NULL_VAL() ((Value){VL_NULL, {.number = 0}})
#define NUM_VAL(value) ((Value){VL_NUM, {.number = value}})
#define OBJ_VAL(value) ((Value){VL_OBJ, {.obj = (Obj*)(value)}})

#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUM(value) ((value).as.number)
#define AS_OBJ(value) ((value).as.obj)

#define IS_BOOL(value) ((value).type == VL_BOOL)
#define IS_NULL(value) ((value).type == VL_NULL)
#define IS_NUM(value) ((value).type == VL_NUM)
#define IS_OBJ(value) ((value).type == VL_OBJ)

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

void initValueArray(ValueArray* array);
int writeValueArray(ValueArray* array, Value value);
void freeValueArray(ValueArray* array);
void printValue(OutType out, Value value);

#endif
