#include <stdio.h>
#include <stdlib.h>
#include "hashmap.h"
#include "value.h"
#include "memory.h"

void initMap(HashMap* map){
	map->numEntries = 0;
	map->capacity = 0;
	map->entries = NULL;
}

void grow(HashMap* map){
	int oldCapacity = map->capacity;
	map->capacity = GROW_CAPACITY(map->capacity);
	map->entries = GROW_ARRAY(map->entries, HashEntry*, oldCapacity, map->capacity);
	
	for(int i = oldCapacity; i < map->capacity; ++i){
		map->entries[i] = NULL;
	}
}

void freeMap(HashMap* map){
	for(int i = 0; i<map->capacity; ++i){
		free(map->entries[i]);
	}
	FREE_ARRAY(HashEntry*, map->entries, map->capacity);
}

static int hashFunction(HashMap* map, const char* key){
	int sum = 0;
	int charIndex = 0;
	while(key[charIndex] != '\0'){
		sum += (uint8_t) key[charIndex];
		++charIndex;
	}
	
	return sum % map->capacity;
}

static HashEntry* includes(HashMap* map, const char* key){
	if(map->capacity == 0 || map->numEntries == 0) return NULL;

	int index = hashFunction(map, key);
	while(map->entries[index] != NULL && map->entries[index]->key != key){
		++index;
	}
	return map->entries[index];
}

void insert(HashMap* map, const char* key, Value value){
	HashEntry* entry = includes(map, key);
	if(entry != NULL){
		entry->value = value;
	}else{
		if(map->numEntries + 1 > map->capacity) grow(map);
		int index = hashFunction(map, key);
		while(map->entries[index] != NULL){
			index = (index + 1) % map->capacity;
		}
		map->entries[index] = malloc(sizeof(HashEntry));
		map->entries[index]->key = key;
		map->entries[index]->value = value;
		++map->numEntries;
	}
}

Value get(HashMap* map, const char* key){
	HashEntry* entry = includes(map, key);
	if(entry != NULL){
		return entry->value;
	}else{
		return NULL_VAL();
	}
}

void set(HashMap* map, const char* key, Value value) {
	HashEntry* entry = includes(map, key);
	if(entry != NULL){
		entry->value = value;
	}
}

void printMap(HashMap* map){
	printf("[");
	if(map->capacity == 0 || map->numEntries == 0) {
		printf("]"); 
		return;
	}
	if(map->entries[0] == NULL){
		printf("X");
	}else{
		printf("%g", AS_NUM((map->entries[0]->value)));
	}

	for(int i = 1; i<map->capacity; ++i){
		if(map->entries[i] == NULL){
			printf(", X");
		}else{
			printf(", %g", AS_NUM((map->entries[i]->value)));
		}
	}
	printf("]\n");
}