#include <stdio.h>
#include <stdlib.h>
#include "hashmap.h"
#include "object.h"
#include "value.h"

void initRect(Shape* shape, Value x, Value y, Value w, Value h){
	HashMap* map = &shape->properties;
	insert(map, copyString("x", 1), x);
	insert(map, copyString("y", 1), y);
	insert(map, copyString("w", 1), w);
	insert(map, copyString("h", 1), h);
}
