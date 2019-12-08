#ifndef scute_svg_h
#define scute_svg_h

#include "value.h"
#include "hashmap.h"
#include "scanner.h"

typedef struct {
	int length;
	char* chars;
} Attribute;

void assignPosition(ObjClosure* close, Value* values, uint8_t numValues);
void assignDimensions(ObjClosure* close, Value* values, uint8_t numValues);

extern void addAttribute(char* key, double value, double index, double line);
extern void addStyle(char* key, double value);
extern void newShape(double id, double type);
extern void paintShape();

void drawShape(HashMap* shapeMap, TKType type);
void renderFrame(ObjClosure* close);
#endif