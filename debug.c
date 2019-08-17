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
	if(offset > 0 && getLine(chunk, offset - 1) == currLine){
		printf("   | ");
	}else{
		printf("%4d ", currLine);
	}

	uint8_t instruction = chunk->code[offset];
	switch(instruction){
		case OP_RETURN:
			return simpleInstruction("OP_RETURN", chunk, offset);
		case OP_CONSTANT:
			return constantInstruction("OP_CONSTANT", chunk, offset);
		default:
			printf("Unknown opcode %d\n", instruction);
			return offset + 1;

	}
}

static int simpleInstruction(const char* name, Chunk* chunk, int offset){	
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

