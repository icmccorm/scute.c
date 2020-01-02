#include "value.h"
#include "hashmap.h"

struct sChunk {
	int count;
	int capacity;
	uint8_t* code;
	ValueArray constants;
	int* opsPerLine;
	int* lineNums;
	int lineCount;
	int lineCapacity;
	int previousLine;
	HashMap* map; 
};