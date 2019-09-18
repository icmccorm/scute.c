#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashmap.h"
#include "value.h"
#include "memory.h"
#include "object.h"

HashEntry* deletedEntry;

void initMap(HashMap* map){
	map->numEntries = 0;
	map->capacity = 0;
	map->entries = NULL;
} 

void grow(HashMap* map){
	int oldCapacity = map->capacity;
	map->capacity = GROW_CAPACITY(map->capacity);
	map->entries = GROW_ARRAY(map->entries, HashEntry* , oldCapacity, map->capacity);
	
	for(int i = oldCapacity; i < map->capacity; ++i){
		map->entries[i] = NULL;
	}
}

void freeMap(HashMap* map){
	for(int i = 0; i<map->capacity; ++i){
		FREE(HashEntry, map->entries[i]);
	}
	FREE_ARRAY(HashEntry*, map->entries, map->capacity);
}

uint32_t hashFunction(char* keyString, int length){
	int sum = 0;
	int charIndex = 0;

	for(int i = 0; i<length; ++i){
		sum += (uint8_t) keyString[i];
	}
}

uint32_t hashIndex(HashMap* map, ObjString* obj){
	return obj->hash % map->capacity;
}

static HashEntry* includes(HashMap* map, ObjString* key){
	if(map->capacity == 0 || map->numEntries == 0) return NULL;

	int index = hashIndex(map, key);
	while(map->entries[index] != NULL && map->entries[index]->key != key){
		++index;
	}
	return map->entries[index];
}

void insert(HashMap* map, ObjString* key, Value value){
	HashEntry* entry = includes(map, key);
	if(entry != NULL){
		entry->value = value;
	}else{
		if(map->numEntries + 1 > map->capacity) grow(map);
		int index = hashIndex(map, key);
		while(map->entries[index] != NULL){
			index = (index + 1) % map->capacity;
		}
		map->entries[index] = ALLOCATE(HashEntry, 1);
		map->entries[index]->key = key;
		map->entries[index]->value = value;
		++map->numEntries;
	}
}


Value getValue(HashMap* map, ObjString* key){
	HashEntry* entry = includes(map, key);
	if(entry != NULL){
		return entry->value;
	}else{
		return NULL_VAL();
	}
}

ObjString* findKey(HashMap* map, char* chars, int length){
	if(map->capacity == 0 || map->numEntries == 0) return NULL;

	uint32_t hash = hashFunction(chars, length);
	uint32_t index = hashFunction(chars, length) % map->capacity;

	while(map->entries[index] == deletedEntry || map->entries[index] != NULL ){
		HashEntry* current = map->entries[index];

		if(current == deletedEntry){
			++index;
		}else{
			if(hash == current->key->hash){
				if(memcpy(current->key->chars, chars, length) == 0) return current->key;
			}else{
				++index;
			}
		}
	}
	return NULL;
}

void set(HashMap* map, ObjString* key, Value value) {
	HashEntry* entry = includes(map, key);
	if(entry != NULL){
		entry->value = value;
	}
}

void delete(HashMap* map, ObjString* key){
	HashEntry* entry = includes(map, key);
	if(entry != NULL){
		entry = deletedEntry;
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