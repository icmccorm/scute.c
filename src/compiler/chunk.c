#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "chunk.h"
#include "memory.h"
#include "value.h"
#include "obj.h"

void initChunk(Chunk* chunk){
	chunk -> count = 0;
	chunk -> capacity = 0;
	chunk -> code = NULL;
	
	initValueArray(&chunk->constants);
	chunk -> opsPerLine = NULL;
	chunk -> lineNums = NULL;
	chunk -> lineCount = -1;
	chunk -> lineCapacity = 0;
	chunk -> previousLine = 0;

	//provides a default value index of 0 for opcodes that include optional arguments
	writeValue(chunk, NULL_VAL(), -1);
}

void writeChunk(Chunk* chunk, uint8_t byte, int line){
	if(line > -1){
		if(chunk->lineCapacity < chunk->lineCount + 1 || chunk->lineCapacity == 0){

			int oldCapacity = chunk->lineCapacity;
			chunk->lineCapacity = GROW_CAPACITY(oldCapacity);
			
			chunk->opsPerLine = GROW_ARRAY(chunk->opsPerLine, int, oldCapacity, chunk->lineCapacity);
			chunk->lineNums = GROW_ARRAY(chunk->lineNums, int, oldCapacity, chunk->lineCapacity);
		}
		// add override for line number -1?
		if(line > chunk->previousLine){
			++(chunk->lineCount);
			chunk->lineNums[chunk->lineCount] = line;
			chunk->opsPerLine[chunk->lineCount] = 1;
			chunk->previousLine = line;
		}else{
			++(chunk->opsPerLine[chunk->lineCount]);
		}
	}
	
	if(chunk->capacity < chunk->count + 1){
		int oldCapacity = chunk->capacity;
		chunk->capacity = GROW_CAPACITY(oldCapacity);
		chunk->code = GROW_ARRAY(chunk->code, uint8_t, oldCapacity, chunk->capacity);

	}
	chunk->code[chunk->count] = byte;
	chunk->count++;
}

void freeChunk(Chunk* chunk){
	FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
	FREE_ARRAY(uint8_t, chunk->opsPerLine, chunk->lineCapacity);
	FREE_ARRAY(uint8_t, chunk->lineNums, chunk->lineCapacity);
	freeValueArray(&chunk->constants);
}

void writeOperatorBundle(Chunk* chunk, OpCode op, uint64_t value, int line){
	writeChunk(chunk, op, line);
	writeVariableData(chunk, value);
}

int writeVariableData(Chunk* chunk, uint64_t value){
	int numBytes = value <= 1 ? 1 : ceil((double)(log(value)/log(2)) / 8); 	
	writeChunk(chunk, (uint8_t) numBytes, -1);
	for(int i = 0; i<numBytes; ++i){
		uint8_t byteAtIndex = (value >> 8*i) & 0xFF;
		writeChunk(chunk, (uint8_t) byteAtIndex, -1);
	}
	return 1 + numBytes;
}

void writeConstant(Chunk* chunk, Value value, int line){
	uint64_t valIndex = pushValueArray(&chunk->constants, value);
	writeOperatorBundle(chunk, OP_CONSTANT, valIndex, line);
}

uint64_t writeValue(Chunk* chunk, Value value, int line){
	return pushValueArray(&chunk->constants, value);
}

int getLine(Chunk* chunk, int opIndex) {
	int runningTotal = 1;
	for(int i = 0; i<chunk->lineCount; ++i){
		runningTotal += chunk->opsPerLine[i];
		if(opIndex <= runningTotal){
			return chunk->lineNums[i];
		}
	}
	return 1;
}