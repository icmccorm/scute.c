#ifndef scute_value_h
#define scute_value_h

#include "common.h"
#include "output.h"
#include "scanner.h"

typedef struct sIntermediate Intermediate;
typedef struct sChunk Chunk;
typedef struct sObj Obj;
typedef struct sObjString ObjString;
typedef struct sObjInstance ObjInstance;
typedef struct sObjChunk ObjChunk;
typedef struct sObjNative ObjNative;
typedef struct sObjColor ObjColor;
typedef struct sObjArray ObjArray;
typedef struct sObjShape ObjShape;

typedef enum {
	VL_NULL,
	VL_BOOL,
	VL_NUM,
	VL_OBJ,
} VLType;

typedef struct {
	VLType type;		//0

	//TODO: Separate this information into a new struct, and link via in index
	uint32_t lineIndex;
	uint32_t inlineIndex;
	Intermediate* stages;
	uint8_t currStage;

	union {				//8
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

#define BOOL_VAL(value) ((Value){VL_BOOL, -1, -1, NULL, 0, {.boolean = value}})
#define NULL_VAL() ((Value){VL_NULL, -1, -1, NULL, 0, {.number = 0}})
#define NUM_VAL(value) ((Value){VL_NUM, -1, -1, NULL, 0, {.number = value}})
#define OBJ_VAL(value) ((Value){VL_OBJ, -1, -1, NULL, 0, {.obj = (Obj*)(value)}})

#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUM(value) ((value).as.number)
#define AS_OBJ(value) ((value).as.obj)

#define IS_BOOL(value) ((value).type == VL_BOOL)
#define IS_NULL(value) ((value).type == VL_NULL)
#define IS_NUM(value) ((value).type == VL_NUM)
#define IS_OBJ(value) ((value).type == VL_OBJ)
#define OBJ_TYPE(value) (AS_OBJ(value)->type)


void printValue(OutType out, Value value);
void printShapeType(OutType out, TKType type);


void initValueArray(ValueArray* array);
void initValueArrayWithCapacity(ValueArray* array, int capacity);
void freeValueArray(ValueArray* array);

int pushValueArray(ValueArray* array, Value value);
Value popValueArray(ValueArray* array);
Value getValueArray(ValueArray* array, int index);
void setValueArray(ValueArray* array, int index, Value val);
Value* getMaxValueByLocation(Value* a, Value* b);
void addValues(ValueArray* array, int numValues, ...);

Value vector(Value x, Value y);

#define VECTOR(x, y) (vector(NUM_VAL(x), NUM_VAL(y)))

#endif
