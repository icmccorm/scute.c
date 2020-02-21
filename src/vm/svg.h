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
extern void em_addAttribute(const char* key, Value* val);

extern void em_addStyle(const char* key, Value* val);
extern void em_addStringStyle(const char* key, Value* val);
extern void em_addTurn(Value* degPtr);
extern void em_addJump(Value* vecPtr);
extern void em_addMove(int x, int y, Value* distPtr);
extern void em_addVertex(Value* vecPtr);

extern void em_newShape(double id, double type);
extern void em_paintShape();

extern void em_setCanvas(Value* width, Value* height, Value* originX, Value* originY);
#endif

void drawShape(HashMap* shapeMap, TKType type);
void renderFrame();
void pushShape(ObjShape* close);
ObjShape* popShape();

#endif