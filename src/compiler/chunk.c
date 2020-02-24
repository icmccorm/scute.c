#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "chunk.h"
#include "memory.h"
#include "value.h"
#include "obj.h"
#include "debug.h"
#include "compiler_defs.h"

void initChunk(Chunk* chunk){
	chunk -> count = 0;
	chunk -> capacity = 0;
	chunk -> code = NULL;
	chunk -> constants = ALLOCATE(ValueArray, 1);
	initValueArray(chunk->constants);
	chunk -> bytesPerLine = NULL;
	chunk -> lineNums = NULL;
	chunk -> lineCount = 0;
	chunk -> lineCapacity = 0;
	chunk -> previousLine = 0;
	//provides a default value index of 0 for opcodes that include optional arguments
	writeValue(chunk, NULL_VAL(), 0);
}

void writeChunk(Chunk* chunk, uint8_t byte, uint32_t line){
	if(line > 0){
		if(chunk->lineCapacity <= chunk->lineCount + 1){
			int oldCapacity = chunk->lineCapacity;
			chunk->lineCapacity = GROW_CAPACITY(oldCapacity);
			chunk->bytesPerLine = GROW_ARRAY(chunk->bytesPerLine, uint32_t, oldCapacity, chunk->lineCapacity);
			chunk->lineNums = GROW_ARRAY(chunk->lineNums, uint32_t, oldCapacity, chunk->lineCapacity);
		}
		if(line > chunk->previousLine){
			chunk->lineNums[chunk->lineCount] = line;
			chunk->bytesPerLine[chunk->lineCount] = 1;
			++chunk->lineCount;
			chunk->previousLine = line;
		}else{
			++(chunk->bytesPerLine[chunk->lineCount-1]);
		}
	}
	if(chunk->capacity < chunk->count + 1){
		int oldCapacity = chunk->capacity;
		chunk->capacity = GROW_CAPACITY(oldCapacity);
		chunk->code = GROW_ARRAY(chunk->code, uint8_t, oldCapacity, chunk->capacity);
	}
	chunk->code[chunk->count] = byte;
	++chunk->count;
}

void freeChunk(Chunk* chunk){
	FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
	FREE_ARRAY(int, chunk->bytesPerLine, chunk->lineCapacity);
	FREE_ARRAY(int, chunk->lineNums, chunk->lineCapacity);
	freeValueArray(chunk->constants);
	FREE(Chunk, chunk);
}

void writeOperatorBundle(Chunk* chunk, OpCode op, uint64_t value, uint32_t line){
	writeChunk(chunk, op, line);
	writeVariableData(chunk, value);
}

int writeVariableData(Chunk* chunk, uint64_t value){
	int numBytes = value <= 1 ? 1 : ceil((double)(log(value)/log(2)) / 8); 	
	writeChunk(chunk, (uint8_t) numBytes, (uint32_t) parser.previous.line);
	for(int i = 0; i<numBytes; ++i){
		uint8_t byteAtIndex = (value >> 8*i) & 0xFF;
		writeChunk(chunk, (uint8_t) byteAtIndex, (uint32_t) parser.previous.line);
	}
	return 1 + numBytes;
}

void writeConstant(Chunk* chunk, Value value, uint32_t line){
	uint64_t valIndex = pushValueArray(chunk->constants, value);
	writeOperatorBundle(chunk, OP_CONSTANT, valIndex, line);
}

uint64_t writeValue(Chunk* chunk, Value value, uint32_t line){
	return pushValueArray(chunk->constants, value);
}

int getLine(Chunk* chunk, uint32_t opIndex) {
	int runningTotal = 1;
	for(int i = 0; i<chunk->lineCount; ++i){
		runningTotal += chunk->bytesPerLine[i];
		if(opIndex <= runningTotal){
			return chunk->lineNums[i];
		}
	}
	return 1;
}