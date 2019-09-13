#ifndef scute_hashmap_h
#define scute_hashmap_h
#include "value.h"

#define LOAD_FACTOR = .75

typedef struct {
	char* key;
	Value value;
} HashEntry;

typedef struct {
	HashEntry** entries;
	int numEntries;
	int capacity;
} HashMap;

void initMap(HashMap* map);
void insert(HashMap* map, char* key, Value value);
Value get(HashMap* map, char* key);
void set(HashMap* map, char* key, Value value);
void grow(HashMap* map);
void printMap(HashMap* map);
#endif