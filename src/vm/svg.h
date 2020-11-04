#ifndef scute_svg_h
#define scute_svg_h

#include "value.h"
#include "hashmap.h"
#include "scanner.h"
#include "package.h"

typedef struct {
	int length;
	char* chars;
} Attribute;

void assignPosition(ObjShape* close, Value* values, uint8_t numValues);
void assignDimensions(ObjShape* close, Value* values, uint8_t numValues);

#ifdef EM_MAIN
extern void em_addAttribute(const char* key, Value* val);
extern void em_addVectorAttr(const char* keyPtr, Value* vecPtr);
extern void em_setCanvas(Value* sizePtr, Value* originPtr);
extern void em_assignAnimation(ObjAnim* anim);
extern void em_addStyle(const char* key, Value* val);
extern void em_addStringStyle(const char* key);
extern void em_initAnimationChunk(ObjAnim* anim);
extern void em_finalizeAnimationChunk();
extern void em_animateValue(char* property, Value* val);
extern void em_addTurn(Value* degPtr);
extern void em_addJump(Value* vecPtr);
extern void em_addMove(Value* distPtr, int* angle);

extern void em_addVertex(Value* vecPtr);
extern void em_addQuadBezier(Value* control, Value* end);
extern void em_addCubicBezier(Value* control1, Value* control2, Value* end);
extern void em_addArc(Value* center, Value* degrees);
extern void em_addMirror(Value* origin, bool x, bool y);
extern void em_newRect(double id);
extern void em_newCirc(double id);
extern void em_newEllip(double id);
extern void em_newLine(double id);
extern void em_newPolygon(double id);
extern void em_newPolyline(double id);
extern void em_newUngon(double id);
extern void em_newPath(double id);

extern void em_paintShape();
extern void em_addColorStyle(const char* key, int length, Value* values);
#endif

void renderAnimationBlocks(CompilePackage* package, int timeIndex);
void renderFrame(CompilePackage* code);
void pushShape(ObjShape* close);
ObjShape* popShape();

#endif