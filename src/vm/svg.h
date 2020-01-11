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
extern void addAttribute(const char* key, double value, double index, double line);
extern void addStringAttribute(const char* key, char* valuePtr, double index, double line);
extern void addStyle(const char* key, double value, double index, double line);
extern void addStringStyle(const char* key, char* valuePtr, double index, double line);
extern void newShape(double id, double type);
extern void paintShape();

#define ATTR(name, value) (addAttribute(name, AS_NUM(value), value.charIndex, value.line));
#define CATTR(name, value) (addStringAttribute(name, value, value.charIndex, value.line));

#define STYLE(name, value) (addStyle(name, AS_NUM(value), value.charIndex, value.line));
#define CSTYLE(name, value) (addStringStyle(name, value, value.charIndex, value.line));
#endif

void drawShape(HashMap* shapeMap, TKType type);
void renderFrame(ObjInstance* close);

#endif