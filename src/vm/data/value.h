#ifndef scute_value_h
#define scute_value_h

#include "common.h"

typedef enum {
	VL_BOOL,
	VL_NULL,
	VL_NUM
} VLType;

typedef struct {
	VLType type;
	union {
		bool boolean;
		double number;
	} as;
} Value;

typedef struct {
	int capacity;
	int count;
	Value * values;
} ValueArray;	

#define BOOL_VAL(value) ((Value){VL_BOOL, {.boolean = value}})
#define NULL_VAL ((Value){VL_NULL, {.number = 0}})
#define NUM_VAL(value) ((Value){VL_NUM, {.number = value}})

#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUM(value) ((value).as.number)

#define IS_BOOL(value) ((value).type == VL_BOOL)
#define IS_NULL(value) ((value).type == VL_NULL)
#define IS_NUM(value) ((value).type == VL_NUM)


void initValueArray(ValueArray* array);
void writeValueArray(ValueArray* array, Value value);
void freeValueArray(ValueArray* array);
void printValue(Value value);

#endif
