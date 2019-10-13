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
void defineCirc(ObjShape* shape, Value cx, Value cy, Value r);
void drawShape(ObjShape* shape);

#ifdef EM_MAIN
extern void addAttribute(char* key, double value);
extern void addStyle(char* key, double value);
extern void newShape(double id, double type);
extern void paintShape();
#endif

#endif