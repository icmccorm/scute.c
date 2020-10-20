#ifndef scute_chunk_h
#define scute_chunk_h

#include "common.h"
#include "hashmap.h"
#include "value.h"

typedef enum {
	CK_FUNC,
	CK_CONSTR,
	CK_MAIN,
	CK_UNDEF
} CKType;

struct sChunk {
	int count;
	int capacity;

	int lineCount;
	int lineCapacity;

	uint8_t* code;
	ValueArray* constants;

	uint32_t* bytesPerLine;
	uint32_t* lineNums;
	uint32_t previousLine;
};

typedef enum {
	OP_ADD,
	OP_SUBTRACT,
	OP_MULTIPLY,
	OP_DIVIDE,
	OP_MODULO,
	OP_RETURN,
	OP_CONSTANT,
	OP_NEGATE,
	OP_NULL,
	OP_TRUE,
	OP_FALSE,
	OP_NOT,
	OP_LESS,
	OP_GREATER,
	OP_EQUALS,
	OP_LESS_EQUALS,
	OP_GREATER_EQUALS,
	OP_PRINT,

	OP_DRAW,
	OP_POP,
	OP_DEF_GLOBAL,
	OP_GET_GLOBAL,
	OP_DEF_LOCAL,
	OP_GET_LOCAL,
	OP_GET_UPVALUE,
	OP_DEF_UPVALUE,

	OP_PUSH_STACKFRAME,
	OP_POP_STACKFRAME,
	OP_MERGE_MAPS,

	OP_JMP_CNT,

	OP_JMP_FALSE,
	OP_LIMIT,
	OP_FRAME_INDEX,
	OP_SCOPE,
	OP_DEREF,
	OP_CALL,
	OP_JMP,

	OP_BUILD_ARRAY,

	OP_INIT_INST,
	OP_PUSH_INST,
	OP_POP_INST,
	OP_LOAD_INST,
	OP_DEF_INST,

	OP_CLOSURE,
	OP_CLOSE_UPVALUE,
	OP_ANIM,
} OpCode;

void initChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte, uint32_t line);
void freeChunk(Chunk* chunk);

int getLine(Chunk* chunk, uint32_t opIndex);

uint32_t writeConstant(Chunk* chunk, Value value, uint32_t line);
void writeOperatorBundle(Chunk* chunk, OpCode op, uint64_t value, uint32_t line);
uint32_t writeValue(Chunk* chunk, Value value, uint32_t line);
int writeVariableData(Chunk* chunk, uint64_t value);

#endif
