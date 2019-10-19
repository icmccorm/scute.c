#include <stdio.h>
#include <stdlib.h>
#include "svg.h"
#include "hashmap.h"
#include "obj.h"
#include "value.h"
#include "vm.h"

void drawShape(ObjShape* shape){
	#ifdef EM_MAIN
		HashEntry* entry = shape->closure.map->first;
		double address = (unsigned) shape;
		newShape(address, shape->type);
		while(entry != NULL){
			switch(entry->value.type){
				case VL_OBJ:
					break;
				default:
					addAttribute(entry->key->chars, AS_NUM(entry->value));
					break;
			}
			entry = entry->next;
		}	
		paintShape();
	#else
	printMap(O_OUT, shape->closure.map);
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
		default:
			break;
	}
}

void defineRect(ObjShape* shape, Value x, Value y, Value width, Value height){
	HashMap* map = shape->closure.map;
	ObjString* xStr = internString("x", 1);
	ObjString* yStr = internString("y", 1);
	ObjString* wStr = internString("width", 5);
	ObjString* hStr = internString("height", 6);
	set(map, xStr, x);
	set(map, yStr, y);
	set(map, wStr, width);
	set(map, hStr, height);
}

void defineCirc(ObjShape* shape, Value cx, Value cy, Value r){
	HashMap* map = shape->closure.map;
	ObjString* cxStr = internString("cx", 2);
	ObjString* cyStr = internString("cy", 2);
	ObjString* rStr = internString("r", 1);
	set(map, cxStr, cx);
	set(map, cyStr, cy);
	set(map, rStr, r);
}

static void initRect(HashMap* map){
	insert(map, internString("x", 1), NULL_VAL());
	insert(map, internString("y", 1), NULL_VAL());
	insert(map, internString("width", 5), NULL_VAL());
	insert(map, internString("height", 6), NULL_VAL());
}

static void initCircle(HashMap* map){
	insert(map, internString("cx", 2), NULL_VAL());
	insert(map, internString("cy", 2), NULL_VAL());
	insert(map, internString("r", 1), NULL_VAL());
}