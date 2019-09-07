#ifndef scute_chunk_h
#define scute_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
	OP_RETURN,
	OP_CONSTANT,
	OP_CONSTANT_LONG,
	OP_NEGATE,
	OP_MULTIPLY,
	OP_DIVIDE,
	OP_SUBTRACT,
	OP_ADD,
	OP_MODULO,
	OP_NULL,
	OP_TRUE,
	OP_FALSE
} OpCode;

typedef struct {
	int count;
	int capacity;
	uint8_t* code;
	ValueArray constants;
	int* opsPerLine;
	int* lineNums;
	int lineCount;
	int lineCapacity;
	int previousLine;
} Chunk;

void initChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte, int line);
void freeChunk(Chunk* chunk);

int getLine(Chunk* chunk, int opIndex);

void writeConstant(Chunk* chunk, Value value, int line);
#endif
