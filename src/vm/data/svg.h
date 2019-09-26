#ifndef scute_svg_h
#define scute_svg_h

#include "value.h"
#include "hashmap.h"

#ifdef EM_MAIN

#endif

typedef enum {
	SP_RECT,
	SP_CIRC,
	SP_LINE,
	SP_POLYL,
	SP_POLYG,
} SPType;

struct sShape{
	SPType type;
	HashMap properties;
};

void initRect(Shape* shape, Value x, Value y, Value w, Value h);

#endif