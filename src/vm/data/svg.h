#ifndef scute_svg_h
#define scute_svg_h

#include "value.h"
#include "hashmap.h"

typedef enum {
	SP_RECT,
	SP_CIRC,
	SP_LINE,
	SP_POLYL,
	SP_POLYG,
} SPType;


void initShape(SPType type, HashMap* map);
void defineRect(ObjShape* shape, Value x, Value y, Value w, Value h);
void drawShape(ObjShape* shape);

#endif