#include <stdlib.h>
#include <stdio.h>
#include "debug.h"
#include "value.h"
#include "chunk.h"
#include "output.h"
#include "obj_def.h"  

static int printInstruction(Chunk* chunk, int offset, int currLine, int prevLine);
static int simpleInstruction(const char* name, int offset);
static int embeddedValueInstruction(const char* name, Chunk* chunk, int offset);
static int embeddedInstruction(const char* name, Chunk* chunk, int offset);
static int jumpInstruction(const char* name, Chunk* chunk, int offset);
static int limitInstruction(const char* name, Chunk* chunk, int offset);
static int scopeInstruction(const char* name, Chunk* chunk, int offset);
static int paramInstruction(const char* name, Chunk* chunk, int offset);
static int closureInstruction(Chunk* chunk, int offset);
static int animInstruction(const char* name, Chunk* chunk, int offset);

void printChunk(Chunk* chunk, const char* name) {
	if(name != NULL) print(O_DEBUG, "== %s ==\n", name);
	int prevLine = 0;
	int currLine = 0;
	for (int offset = 0; offset < chunk->count;) {
		currLine = getLine(chunk, offset);
		offset = printInstruction(chunk, offset, currLine, prevLine);
		prevLine = currLine;
	}
	if(name != NULL) print(O_DEBUG, "======\n");
}

static int printInstruction(Chunk* chunk, int offset, int currLine, int prevLine){
	print(O_DEBUG, "%4d ", offset);
	
	if(currLine == prevLine){
		print(O_OUT, "    |   ");
	}else{
		print(O_DEBUG, "%4d    ", currLine);
	}

	uint8_t instruction = chunk->code[offset];
	switch(instruction){
		case OP_POWER:
			return simpleInstruction("OP_POWER", offset);
		case OP_INTERPOLATE: 
			return simpleInstruction("OP_INTERPOLATE", offset);
		case OP_ANIM:
			return animInstruction("OP_ANIM", chunk, offset);
		case OP_CLOSE_UPVALUE:
			return simpleInstruction("OP_CLOSE_UPVALUE", offset);
		case OP_POP_INST:
			return 1 + simpleInstruction("OP_POP_INST", offset);
		case OP_PUSH_INST:
			return simpleInstruction("OP_PUSH_INST", offset);
		case OP_INIT_INST:
			return simpleInstruction("OP_INIT_INST", offset);
		case OP_DIVIDE:
			return simpleInstruction("OP_DIVIDE", offset);
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
			return embeddedValueInstruction("OP_CONSTANT", chunk, offset);
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
		case OP_LOAD_INST:
			return simpleInstruction("OP_LOAD_INST", offset);
		case OP_DEF_GLOBAL:
			return embeddedValueInstruction("OP_DEF_GLOBAL", chunk, offset);
		case OP_GET_GLOBAL:
			return embeddedValueInstruction("OP_GET_GLOBAL", chunk, offset);
		case OP_GET_UPVALUE:
			return embeddedValueInstruction("OP_GET_UPVALUE", chunk, offset);
		case OP_DEF_UPVALUE:
			return embeddedValueInstruction("OP_DEF_UPVALUE", chunk, offset);
		case OP_BUILD_ARRAY:
			return embeddedInstruction("OP_BUILD_ARRAY", chunk, offset);
		case OP_DEF_LOCAL:
			return embeddedInstruction("OP_DEF_LOCAL", chunk, offset);
		case OP_GET_LOCAL:
			return embeddedInstruction("OP_GET_LOCAL", chunk, offset);
		case OP_JMP_FALSE:
			return jumpInstruction("OP_JMP_FALSE", chunk, offset);
		case OP_JMP:
			return jumpInstruction("OP_JMP", chunk, offset);
		case OP_JMP_CNT:
			return jumpInstruction("OP_JMP_CNT", chunk, offset);
		case OP_LIMIT:
			return limitInstruction("OP_LIMIT", chunk, offset);
		case OP_DRAW:
			return simpleInstruction("OP_DRAW", offset);
		case OP_POP:
			return simpleInstruction("OP_POP", offset);
		case OP_CALL:
			return paramInstruction("OP_CALL", chunk, offset);
		case OP_CLOSURE:
			return closureInstruction(chunk, offset);
		case OP_FRAME_INDEX:
			return simpleInstruction("OP_FRAME_INDEX", offset);
		case OP_DEREF:
			return embeddedValueInstruction("OP_DEREF", chunk, offset);
		case OP_DEF_INST:
			// add a 1 to account for the 1-byte pop mode flag
			// TODO: add a more flexible option for including single byte flags
			return 1 + embeddedInstruction("OP_DEF_INST", chunk, offset);
		default:
			print(O_DEBUG, "Unknown opcode %d\n", instruction);
			return offset + 1;
	}
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

static int closureInstruction(Chunk* chunk, int offset){
	uint8_t numBytes = chunk->code[offset + 1];
	uint32_t constant = readEmbeddedInteger(chunk, numBytes, offset);
	offset += numBytes + 2 + 1;

	print(O_DEBUG, "%-16s %4d ", "OP_CLOSURE", constant);
	printValue(O_DEBUG, chunk->constants->values[constant]);
	ObjChunk* chunkObj = (ObjChunk*) AS_OBJ(chunk->constants->values[constant]);
	for (int i = 0; i< chunkObj->upvalueCount; ++i){
		int isLocal = chunk->code[offset++];

		uint8_t numBytes = chunk->code[offset];
		int index = readEmbeddedInteger(chunk, numBytes, offset);
		offset += numBytes + 1;

		//int index = chunk->code[offset++];
		print(O_DEBUG, "%04d	|				%s %d\n", offset - 2, isLocal ? "local" : "upvalue", index);
	}	
	return offset;
}

static int simpleInstruction(const char* name, int offset){	
	print(O_DEBUG, "%s\n", name);
	return offset + 1;
}

static int paramInstruction(const char* name, Chunk* chunk, int offset){
	print(O_DEBUG, "%s(%d)\n", name, chunk->code[offset + 1]);
	return offset + 2;
}

static int jumpInstruction(const char* name, Chunk* chunk, int offset){
	print(O_DEBUG, "%-16s  ", name);
	signed short testValue = (signed short)((chunk->code[offset + 1]<<8) | (chunk->code[offset + 2]));
	print(O_DEBUG, "%d\n",testValue);
	return offset + 3;
}

static int limitInstruction(const char* name, Chunk* chunk, int offset){
	uint8_t numLowerBytes = chunk->code[offset + 1];
	int lowerValue = readEmbeddedInteger(chunk, numLowerBytes, offset);
	offset = offset + 1 + numLowerBytes;

	uint8_t numUpperBytes = chunk->code[offset + 1];
	int upperValue = readEmbeddedInteger(chunk, numUpperBytes, offset);
	offset = offset + 1 + numUpperBytes;

	print(O_DEBUG, "%-16s %4d %4d\n", name, lowerValue, upperValue);
	return offset + 3;
}	

static int embeddedValueInstruction(const char* name, Chunk* chunk, int offset){
	uint8_t numBytes = chunk->code[offset + 1];
	uint32_t valIndex = readEmbeddedInteger(chunk, numBytes, offset);

	Value* val = &(chunk->constants->values[valIndex]);
	offset = offset + 1 + numBytes;
	print(O_DEBUG, "%-16s", name, valIndex);
	print(O_DEBUG, "%4d ", valIndex);
	printValue(O_DEBUG, *val);
	if(val->lineIndex != -1) {
		print(O_DEBUG, "*(%d, %d)", val->lineIndex, val->inlineIndex);
	}
	print(O_DEBUG, "\n");
	return offset + 1;
}

static int embeddedInstruction(const char* name, Chunk* chunk, int offset){
	uint8_t numBytes = chunk->code[offset + 1];
	uint32_t valIndex = readEmbeddedInteger(chunk, numBytes, offset);

	offset = offset + 1 + numBytes;
	print(O_DEBUG, "%-16s %4d ", name, valIndex);
	print(O_DEBUG, "\n");
	return offset + 1;
}

static int animInstruction(const char* name, Chunk* chunk, int offset){
	uint8_t numBytes = chunk->code[offset + 1];
	uint32_t valIndex = readEmbeddedInteger(chunk, numBytes, offset);
	offset = offset + 1 + numBytes;

	uint16_t min = (chunk->code[offset] << 8) & chunk->code[offset+1];
	uint16_t max = (chunk->code[offset+2] << 8) & chunk->code[offset+3];
	offset = offset + 4;

	print(O_DEBUG, "%-16s %4d [%d, %d] ", name, valIndex, min, max);
	printValue(O_DEBUG, chunk->constants->values[valIndex]);
	print(O_DEBUG, "\n");
	return offset + 1;
}