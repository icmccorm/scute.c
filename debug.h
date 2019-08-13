#ifndef scute_debug_h
#define scute_debug_h

#include "chunk.h"
void printChunk(Chunk* chunk, const char* name);
int printInstruction(Chunk* chunk, int offset);

int simpleInstruction(const char* name, int offset);
#endif
