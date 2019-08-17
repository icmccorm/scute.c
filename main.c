#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "value.h"

int main(int argc, const char* argv[]){
	Chunk chunk;
	initChunk(&chunk);
	writeChunk(&chunk, OP_RETURN, 44);
	writeConstant(&chunk, 1.2, 121);
	printChunk(&chunk, "test chonker");
	freeChunk(&chunk);
	return 0;
}




