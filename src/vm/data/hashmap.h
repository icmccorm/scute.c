#include "value.h"
typedef struct {
	const char* key;
	Value value;
} HashEntry;

typedef struct {
	HashEntry** entries;
	int numEntries;
	int capacity;
} HashMap;

void initMap(HashMap* map);
void insert(HashMap* map, const char* key, Value value);
Value get(HashMap* map, const char* key);
void set(HashMap* map, const char* key, Value value);
void grow(HashMap* map);
void printMap(HashMap* map);