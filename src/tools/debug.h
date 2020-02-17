#ifndef scute_debug_h
#define scute_debug_h

#include "chunk.h"
#include "value.h"

void printChunk(Chunk* chunk, const char* name);
int getLine(Chunk* chunk, uint32_t opIndex);

#endif
