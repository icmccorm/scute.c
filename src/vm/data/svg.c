#include <stdio.h>
#include <stdlib.h>
#include "svg.h"
#include "hashmap.h"
#include "obj.h"
#include "value.h"

void drawShape(ObjShape* shape){
	#ifdef EM_MAIN
		extern void addProperty(char* key, double value);
		extern void paintShape();
		HashEntry* entry = shape->closure.map.first;
		while(entry != NULL){
			switch(entry->value.type){
				case VL_OBJ:
					break;
				default:
					addProperty(entry->key->chars, AS_NUM(entry->value));
					break;
			}
			entry = entry->next;
		}	
		paintShape();
	#else
	printMap(O_OUT, &shape->closure.map);
	#endif
}

static void initRect();
static void initCircle();

void initShape(SPType type, HashMap* map){
	switch(type){
		case SP_RECT:
			initRect(map);
			break;
		case SP_CIRC:
			initCircle(map);
			break;
	}
}

void defineRect(ObjShape* shape, Value x, Value y, Value width, Value height){
	HashMap* map = &shape->closure.map;
	set(map, internString("x", 1), x);
	set(map, internString("y", 1), y);
	set(map, internString("width", 1), width);
	set(map, internString("height", 1), height);
}

static void initRect(HashMap* map){
	insert(map, internString("x", 1), NULL_VAL());
	insert(map, internString("y", 1), NULL_VAL());
	insert(map, internString("width", 1), NULL_VAL());
	insert(map, internString("height", 1), NULL_VAL());
}

static void initCircle(HashMap* map){
	insert(map, internString("cx", 1), NULL_VAL());
	insert(map, internString("cy", 1), NULL_VAL());
	insert(map, internString("r", 1), NULL_VAL());
}