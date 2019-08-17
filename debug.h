#ifndef scute_debug_h
#define scute_debug_h

#include "chunk.h"
void printChunk(Chunk* chunk, const char* name);
int printInstruction(Chunk* chunk, int offset);

static int simpleInstruction(const char* name, Chunk* chunk, int offset);
static int constantInstruction(const char* name, Chunk* chunk, int offset);
int getLine(Chunk* chunk, int opIndex);

#endif
