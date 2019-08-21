#include <stdlib.h>
#include <stdio.h>
#include "debug.h"
#include "value.h"
#include "chunk.h"


void printChunk(Chunk* chunk, const char* name) {
	printf("== %s ==\n", name);
	for (int offset = 0; offset < chunk->count;) {
		offset = printInstruction(chunk, offset);
	}
}

int printInstruction(Chunk* chunk, int offset){
	printf("%4d ", offset);
	
	int currLine = getLine(chunk, offset);
	if(currLine == -1 || offset > 0 && getLine(chunk, offset - 1) == currLine){
		printf("   | ");
	}else{
		printf("%4d ", currLine);
	}

	uint8_t instruction = chunk->code[offset];
	switch(instruction){
		case OP_RETURN:
			return simpleInstruction("OP_RETURN", offset);
		case OP_NEGATE:
			return simpleInstruction("OP_NEGATE", offset);
		case OP_ADD:
			return simpleInstruction("OP_ADD", offset);
		case OP_SUBTRACT:
			return simpleInstruction("OP_SUBTRACT", offset);
		case OP_MULTIPLY:
			return simpleInstruction("OP_MULTIPLY", offset);
		case OP_MODULO:
			return simpleInstruction("OP_MODULO", offset);
		case OP_CONSTANT:
			return constantInstruction("OP_CONSTANT", chunk, offset);
		case OP_CONSTANT_LONG:
			return longConstantInstruction("OP_CONSTANT_LONG", chunk, offset);
		default:
			printf("Unknown opcode %d\n", instruction);
			return offset + 1;

	}
}

static int simpleInstruction(const char* name, int offset){	
	printf("%s\n", name);
	return offset + 1;
}

static int constantInstruction(const char* name, Chunk* chunk, int offset){
	uint8_t constant = chunk->code[offset + 1];
	printf("%-16s %4d '", name, constant);
	printValue(chunk->constants.values[constant]);
	printf("'\n");
	return offset + 2;
}

static int longConstantInstruction(const char* name, Chunk* chunk, int offset){
	uint8_t bytes[3];
	uint32_t valIndex = 0;
	
	for(int i = 0; i< 3; ++i){
		uint8_t currByte = chunk->code[offset + 1 + i];
		int32_t append = (int32_t) currByte << i*8;
		valIndex += append;
	}
	printf("%-16s %4d '", name, valIndex);
	printValue(chunk->constants.values[valIndex]);
	printf("'\n");
	return offset + 4;
}

int getLine(Chunk* chunk, int opIndex) {
	int runningTotal = 0;
	for(int i = 0; i<chunk->lineCount; ++i){
		runningTotal += chunk->opsPerLine[i];
		if(opIndex <= runningTotal){
			return chunk->lineNums[i];
		}
	}
	return 0;
}

