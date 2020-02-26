#ifndef scute_color_h
#define scute_color_h

#include "common.h"
#include "value.h"

typedef enum {
	CL_RGB,
	CL_HSL,
	CL_CMYK,
} CLType;

ObjArray* allocateColor(double r, double g, double b, double a);
#define RGB(r, g, b) (OBJ_VAL(allocateColor(r, g, b, 255)))
#define RGBA(r, g, b, a) (OBJ_VAL(allocateColor(r, g, b, a)))
#endif
