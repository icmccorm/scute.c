#ifndef scute_hashmap_h
#define scute_hashmap_h
#include "value.h"

#define LOAD_FACTOR = .75

typedef struct {
	ObjString* key;
	Value value;
} HashEntry;

typedef struct {
	HashEntry** entries;
	int numEntries;
	int capacity;
} HashMap;

void initMap(HashMap* map);
void freeMap(HashMap* map);
void insert(HashMap* map, ObjString* key, Value value);
Value getValue(HashMap* map, ObjString* key);
void set(HashMap* map, ObjString* key, Value value);
void grow(HashMap* map);
void printMap(HashMap* map);
uint32_t hashFunction(char* keyString, int length);
uint32_t hashIndex(HashMap* map, ObjString* obj);
ObjString* findKey(HashMap* map, char* chars, int length);


#endif