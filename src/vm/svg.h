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
extern void addAttribute(const char* key, double value, int index);
extern void addStringAttribute(const char* key, char* valuePtr, int index);

extern void addStyle(const char* key, double value, int index);
extern void addStringStyle(const char* key, char* valuePtr, int index);

extern void newShape(double id, double type);
extern void paintShape();

#define ATTR(name, value) (addAttribute(name, AS_NUM(value), value.valIndex));
#define CATTR(name, value) (addStringAttribute(name, AS_CSTRING(value), value.valIndex));

#define STYLE(name, value) (addStyle(name, AS_NUM(value), value.valIndex));
#define CSTYLE(name, value) (addStringStyle(name, AS_CSTRING(value), value.valIndex));
#endif

void drawShape(HashMap* shapeMap, TKType type);
void renderFrame();
void pushShape(ObjInstance* close);
ObjInstance* popShape();



#endif