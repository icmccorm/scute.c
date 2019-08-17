#include <stdlib.h>
#include <stdio.h>
#include "chunk.h"
#include "memory.h"
#include "value.h"

void initChunk(Chunk* chunk){
	chunk -> count = 0;
	chunk -> capacity = 0;
	chunk -> code = NULL;
	initValueArray(&chunk->constants);
		
	chunk -> opsPerLine = NULL;
	chunk -> lineNums = NULL;
	chunk -> lineCount = 0;
	chunk -> lineCapacity = 0;
}

void writeChunk(Chunk* chunk, uint8_t byte, int line){
	if(chunk->lineCapacity < chunk->lineCount + 1){

		int oldCapacity = chunk->lineCapacity;
		chunk->lineCapacity = GROW_CAPACITY(oldCapacity);
		
		chunk->opsPerLine = GROW_ARRAY(chunk->opsPerLine, int, oldCapacity, chunk->lineCapacity);
		chunk->lineNums = GROW_ARRAY(chunk->lineNums, int, oldCapacity, chunk->lineCapacity);
	
		if(chunk->lineCount = 0) ++chunk->lineCount;
	}
	
	if(line > chunk->lineNums[chunk->lineCount-1]){
		chunk->lineNums[chunk->lineCount] = line;
		++chunk->lineCount;
		chunk->opsPerLine[chunk->lineCount] = 1;		

	}else{
		++chunk->opsPerLine[chunk->lineCount-1];
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
	FREE_ARRAY(uint8_t, chunk->opsPerLine, chunk->lineCount);
	FREE_ARRAY(uint8_t, chunk->lineNums, chunk->lineCount);
	freeValueArray(&chunk->constants);

	initChunk(chunk);
}

void writeConstant(Chunk* chunk, Value value, int line){
	writeValueArray(&chunk->constants, value);
	writeChunk(chunk, chunk->constants.count-1, line);
}
