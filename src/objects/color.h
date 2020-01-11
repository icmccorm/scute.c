#ifndef scute_color_h
#define scute_color_h

#include "common.h"
#include "value.h"

typedef enum {
	CL_RGB,
	CL_HSL,
	CL_CMYK,
} CLType;

typedef struct {
	CLType colorType;
} Color;

typedef struct {
	Color color;
	Value h;
	Value s;
	Value l;
} ColorHSL;

typedef struct {
	Color color;
	Value r;
	Value g;
	Value b;
} ColorRGB;

typedef struct {
	Color color;
	Value c;
	Value m;
	Value y;
	Value k;
} ColorCMYK;

char* allocateColorString(Color* cl);
void addColorAttribute(const char* key, Color* cl);
void printColor(OutType out, Color* cl);

#ifdef EM_MAIN
#define COLOR_ATTR(key, color) (addColorAttribute(key, color))
#endif


#endif
