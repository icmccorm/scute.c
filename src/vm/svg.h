#ifndef scute_svg_h
#define scute_svg_h

#include "value.h"
#include "hashmap.h"
#include "scanner.h"

typedef struct {
	int length;
	char* chars;
} Attribute;

void assignPosition(ObjShape* close, Value* values, uint8_t numValues);
void assignDimensions(ObjShape* close, Value* values, uint8_t numValues);

#ifdef EM_MAIN
extern void addAttribute(const char* key, Value* val);

extern void addStyle(const char* key, Value* val);
extern void addStringStyle(const char* key, Value* val);

extern void newShape(double id, double type);
extern void paintShape();

extern void setCanvas(Value* width, Value* height, Value* originX, Value* originY);
#endif

void drawShape(HashMap* shapeMap, TKType type);
void renderFrame();
void pushShape(ObjShape* close);
ObjShape* popShape();

#endif