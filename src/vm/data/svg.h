#ifndef scute_svg_h
#define scute_svg_h

#include "value.h"

typedef enum {
	SP_RECT,
	SP_CIRC,
	SP_LINE,
	SP_POLYL,
	SP_POLYG,
} SPType;

struct sShape{
	SPType type;
};

struct sRect{
	Shape shape;
	int x;
	int y;
	int width;
	int height;
};

struct sCircle{
	Shape shape;
	int cx;
	int cy;
	int r;
};

#define AS_RECT(value) ((Rect*) value)

#endif