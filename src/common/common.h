#ifndef scute_common_h
#define scute_common_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
	INTERPRET_OK,
	INTERPRET_COMPILE_ERROR,
	INTERPRET_RUNTIME_ERROR,
} InterpretResult;

#endif
