#include <stdlib.h>
#include <stdio.h>
#include "debug.h"
#include "value.h"
#include "chunk.h"
#include "output.h"

void printChunk(Chunk* chunk, const char* name) {
	print(O_DEBUG, "== %s ==\n", name);
	for (int offset = 0; offset < chunk->count;) {
		offset = printInstruction(chunk, offset);
	}
}

static int simpleInstruction(const char* name, int offset);
static int embeddedInstruction(const char* name, Chunk* chunk, int offset);

int printInstruction(Chunk* chunk, int offset){
	print(O_DEBUG, "%4d ", offset);
	
	int currLine = getLine(chunk, offset);
	if(currLine == -1 || offset > 0 && getLine(chunk, offset - 1) == currLine){
		print(O_DEBUG, "   | ");
	}else{
		print(O_DEBUG, "%4d ", currLine);
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
			return embeddedInstruction("OP_CONSTANT", chunk, offset);
		case OP_TRUE:
			return simpleInstruction("OP_TRUE", offset);
		case OP_FALSE:
			return simpleInstruction("OP_FALSE", offset);
		case OP_NULL:
			return simpleInstruction("OP_NULL", offset);
		case OP_NOT:
			return simpleInstruction("OP_NOT", offset);
		case OP_LESS:
			return simpleInstruction("OP_LESS", offset);
		case OP_GREATER:
			return simpleInstruction("OP_GREATER", offset);
		case OP_LESS_EQUALS:
			return simpleInstruction("OP_LESS_EQUALS", offset);
		case OP_GREATER_EQUALS:
			return simpleInstruction("OP_GREATER_EQUALS", offset);
		case OP_EQUALS:
			return simpleInstruction("OP_EQUALS", offset);
		case OP_PRINT:
			return simpleInstruction("OP_PRINT", offset);
		case OP_PI:
			return simpleInstruction("OP_PI", offset);
		case OP_TAU:
			return simpleInstruction("OP_TAU", offset);
		case OP_E:
			return simpleInstruction("OP_E", offset);
		case OP_SEPARATOR:
			return simpleInstruction("OP_SEPARATOR", offset);
		case OP_DEF_GLOBAL:
			return embeddedInstruction("OP_DEF_GLOBAL", chunk, offset);
		case OP_GET_GLOBAL:
			return embeddedInstruction("OP_GET_GLOBAL", chunk, offset);
		case OP_RECT:
			return simpleInstruction("OP_RECT", offset);
		case OP_DRAW:
			return simpleInstruction("OP_DRAW", offset);
		default:
			print(O_DEBUG, "Unknown opcode %d\n", instruction);
			return offset + 1;
	}
}

static int simpleInstruction(const char* name, int offset){	
	print(O_DEBUG, "%s\n", name);
	return offset + 1;
}

static uint32_t readEmbeddedInteger(Chunk* chunk, int numBytes, int offset){
	uint8_t bytes[numBytes];
	uint32_t valIndex = 0;
	for(int i = 1; i<=numBytes; ++i){
		uint8_t currByte = chunk->code[offset+1+i];
		int32_t append = (uint32_t) currByte << (i-1)*8;
		valIndex += append;
	}
	return valIndex;
}

static int embeddedInstruction(const char* name, Chunk* chunk, int offset){
	uint8_t numBytes = chunk->code[offset + 1];
	uint32_t valIndex = readEmbeddedInteger(chunk, numBytes, offset);

	print(O_DEBUG, "%-16s %4d '", name, valIndex);
	printValue(O_DEBUG, chunk->constants.values[valIndex]);
	print(O_DEBUG, "'\n");
	return offset + 2 + numBytes;
	/*
	for(int i = 0; i< 3; ++i){
		uint8_t currByte = chunk->code[offset + 1 + i];
		int32_t append = (int32_t) currByte << i*8;
		valIndex += append;
	}
	print(O_DEBUG, "%-16s %4d '", name, valIndex);
	printValue(O_DEBUG, chunk->constants.values[valIndex]);
	print(O_DEBUG, "'\n");
	return offset + 4;*/
}