#ifndef scute_common_h
#define scute_common_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

extern bool DEBUG_STACK;
extern int numBytesAllocated;

typedef enum {
	INTERPRET_OK,
	INTERPRET_COMPILE_ERROR,
	INTERPRET_RUNTIME_ERROR,
} InterpretResult;

#endif
