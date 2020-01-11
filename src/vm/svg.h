#ifndef scute_svg_h
#define scute_svg_h

#include "value.h"
#include "hashmap.h"
#include "scanner.h"

typedef struct {
	int length;
	char* chars;
} Attribute;

void assignPosition(ObjInstance* close, Value* values, uint8_t numValues);
void assignDimensions(ObjInstance* close, Value* values, uint8_t numValues);

#ifdef EM_MAIN
extern void addAttribute(char* key, double value, double index, double line);
extern void addStringAttribute(char* key, char* valuePtr, double index, double line);

#define ATTR(name, value) (addAttribute(name, value, value.charIndex, value.line));
#define CATTR(name, value) (addStringAttribute(name, value, value.charIndex, value.line));
#endif

extern void addStyle(char* key, double value);
extern void newShape(double id, double type);
extern void paintShape();

void drawShape(HashMap* shapeMap, TKType type);
void renderFrame(ObjInstance* close);


#endif