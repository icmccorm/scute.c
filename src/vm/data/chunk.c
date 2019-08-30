#include <stdlib.h>
#include <stdio.h>
#include "chunk.h"
#include "memory.h"
#include "value.h"
#include "math.h"

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
	
		if(chunk->lineCount == 0) ++chunk->lineCount;
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

	int numBytes = ceil((double)(log(chunk->constants.count-1)/log(2)) / 8); 	
	/* It can be assumed that the average programmer will never reach the overflow limit
	 * of 2^24 unique literals of type number, but who really knows for sure?
	 * with this assumption, having two separate types of constant instructions is just as
	 * efficient as having an extra preceeding byte to denote the quantity of proceeding bytes,
	 * up to a certain point.
	 */

	if(numBytes > 1){
		writeChunk(chunk, OP_CONSTANT_LONG, line);
		int constIndex = chunk->constants.count-1;

		for(int i = 0; i<3; ++i){
			uint8_t byteAtIndex = (constIndex >> 8*i) & 0xFF;
//			printf("byte: %x\n", byteAtIndex);
			writeChunk(chunk, (uint8_t) byteAtIndex, -1);

		}
	
	}else{
		writeChunk(chunk, OP_CONSTANT, line);
		writeChunk(chunk, chunk->constants.count - 1, -1);

	}
}
