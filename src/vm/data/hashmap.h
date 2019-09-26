#ifndef scute_hashmap_h
#define scute_hashmap_h
#include "value.h"
#include "output.h"

#define LOAD_FACTOR = .75

typedef struct HashEntry {
	ObjString* key;
	Value value;
	struct HashEntry * next;
} HashEntry;

typedef struct {
	HashEntry* entries;
	int numEntries;
	int capacity;
	HashEntry* first;
	HashEntry* previous;
} HashMap;

void initMap(HashMap* map);
void freeMap(HashMap* map);
void insert(HashMap* map, ObjString* key, Value value);
Value getValue(HashMap* map, ObjString* key);
void set(HashMap* map, ObjString* key, Value value);
void grow(HashMap* map);
uint32_t hashFunction(char* keyString, int length);
int hashIndex(HashMap* map, ObjString* obj);
ObjString* findKey(HashMap* map, char* chars, int length);
void printMap(OutType out, HashMap* map);

#endif