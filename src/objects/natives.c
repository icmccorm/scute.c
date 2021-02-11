#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include "value.h"
#include "common.h"
#include "vm.h"
#include "natives.h"
#include "obj.h"
#include "svg.h"

#define POLY_COMPATIBLE(objShape) (objShape->shapeType == TK_POLYLINE || objShape->shapeType == TK_POLYGON || objShape->shapeType == TK_PATH || objShape->shapeType == TK_UNGON)

Value nativeRandom(Value* params, int numParams){
	switch(numParams){
		case 1:{
			if(IS_NUM(params[0])){
				return NUM_VAL(floor(drand48()*AS_NUM(params[0])));
			}else{
				return NUM_VAL(drand48());
			}
		} break;
		case 0:{
			return NUM_VAL(drand48());
		} break;
		default:{
			if(IS_NUM(params[0]) && IS_NUM(params[1])){
				double highVal = AS_NUM(params[1]);
				double lowVal = AS_NUM(params[0]);
				double inRange = floor(drand48()*(highVal - lowVal) + lowVal);
				return NUM_VAL(inRange);
			}else{
				return NUM_VAL(drand48());
			}
		} break;
	}
	return NULL_VAL();
}

Value nativePrint(Value* params, int numParams){
	if(numParams > 0){
		for(int i = 0; i<numParams; ++i){
			printValue(O_OUT, params[i]);
			print(O_OUT, "\n");
		}
	}else{
		print(O_OUT, "\n");
	}
	return NULL_VAL();
}

Value nativeSine(Value* params, int numParams){
	if(numParams > 0) {
		Value operand = params[0];
		double number = AS_NUM(operand);
		return NUM_VAL(sin(number));
	}else{
		runtimeError("Missing operand.");
		return NULL_VAL();
	}
}
Value nativeCosine(Value* params, int numParams){
	if(numParams > 0) {
		Value operand = params[0];
		double number = AS_NUM(operand);
		return NUM_VAL(cos(number));
	}else{
		runtimeError("Missing operand.");
		return NULL_VAL();
	}
}
Value nativeTangent(Value* params, int numParams){
	if(numParams > 0) {
		Value operand = params[0];
		double number = AS_NUM(operand);
		return NUM_VAL(tan(number));
	}else{
		runtimeError("Missing operand.");
		return NULL_VAL();
	}
}
Value nativeArccos(Value* params, int numParams){
	if(numParams > 0) {
		Value operand = params[0];
		double number = AS_NUM(operand);
		return NUM_VAL(acos(number));
	}else{
		runtimeError("Missing operand.");
		return NULL_VAL();
	}
}
Value nativeArcsin(Value* params, int numParams){
	if(numParams > 0) {
		Value operand = params[0];
		double number = AS_NUM(operand);
		return NUM_VAL(asin(number));
	}else{
		runtimeError("Missing operand.");
		return NULL_VAL();
	}
}
Value nativeArctan(Value* params, int numParams){
	if(numParams > 0) {
		Value operand = params[0];
		double number = AS_NUM(operand);
		return NUM_VAL(atan(number));
	}else{
		runtimeError("Missing operand.");
		return NULL_VAL();
	}
}
Value nativeHypcos(Value* params, int numParams){
	if(numParams > 0) {
		Value operand = params[0];
		double number = AS_NUM(operand);
		return NUM_VAL(cosh(number));
	}else{
		runtimeError("Missing operand.");
		return NULL_VAL();
	}
}
Value nativeHypsin(Value* params, int numParams){
	if(numParams > 0) {
		Value operand = params[0];
		double number = AS_NUM(operand);
		return NUM_VAL(sinh(number));
	}else{
		runtimeError("Missing operand.");
		return NULL_VAL();
	}
}

Value nativeDegrees(Value* params, int numParams){
	if(numParams > 0) {
		Value operand = params[0];
		double number = AS_NUM(operand);
		return NUM_VAL(number * 180 / M_PI);
	}else{
		runtimeError("Missing operand.");
		return NULL_VAL();
	}
}

Value nativeRadians(Value* params, int numParams){
	if(numParams > 0) {
		Value operand = params[0];
		double number = AS_NUM(operand);
		return NUM_VAL(number * M_PI / 180);
	}else{
		runtimeError("Missing operand.");
		return NULL_VAL();
	}
}

Value nativeSqrt(Value* params, int numParams){
	if(numParams > 0) {
		Value operand = params[0];
		double number = AS_NUM(operand);
		return NUM_VAL(sqrt(number));
	}else{
		runtimeError("Missing operand.");
		return NULL_VAL();
	}
}

Value jump(Value* params, int numParams){
	ObjShape* jumpInstance = allocateShape(NULL, TK_JUMP);
	add(jumpInstance->instance.map, string("position"), VECTOR(0, 0));

	ObjInstance* current = currentInstance();
	if(current && current->type == INST_SHAPE){
		ObjShape* shape = (ObjShape*) current;
		if(POLY_COMPATIBLE(shape)){
			addSegment(shape, jumpInstance);
			return OBJ_VAL((ObjInstance*) jumpInstance);
		}
	}
	runtimeError("Only poly-type shapes and paths can accept move commands.");
	return OBJ_VAL((ObjInstance*) jumpInstance);
}

Value move(Value* params, int numParams){
	ObjShape* moveInstance = allocateShape(NULL, TK_MOVE);
	add(moveInstance->instance.map, string("distance"), NUM_VAL(0));

	ObjInstance* current = currentInstance();
	if(current && current->type == INST_SHAPE){
		ObjShape* shape = (ObjShape*) current;
		if(POLY_COMPATIBLE(shape)){
			addSegment(shape, moveInstance);
			return OBJ_VAL((ObjInstance*) moveInstance);
		}
	}
	runtimeError("Only poly-type shapes and paths can accept move commands.");
	return OBJ_VAL((ObjInstance*) moveInstance);
}

Value vertex(Value* params, int numParams) {
	ObjShape* vertexInstance = allocateShape(NULL, TK_VERTEX);
	add(vertexInstance->instance.map, string("position"), VECTOR(0, 0));

	ObjInstance* current = currentInstance();
	if(current && current->type == INST_SHAPE){
		ObjShape* shape = (ObjShape*) current;
		if(POLY_COMPATIBLE(shape)){
			addSegment(shape, vertexInstance);
			return OBJ_VAL((ObjInstance*) vertexInstance);
		}
	}
	runtimeError("Only poly-type shapes and paths can accept vertices.");
	return OBJ_VAL((ObjInstance*) vertexInstance);
}

Value turn(Value* params, int numParams){
	ObjShape* turnInstance = allocateShape(NULL, TK_TURN);
	add(turnInstance->instance.map, string("degrees"), NUM_VAL(0));

	ObjInstance* current = currentInstance();
	if(current && current->type == INST_SHAPE){
		ObjShape* shape = (ObjShape*) current;
		if(POLY_COMPATIBLE(shape)){
			addSegment(shape, turnInstance);
			return OBJ_VAL((ObjInstance*) turnInstance);
		}
	}
	runtimeError("Only poly-type shapes and paths can accept turn commands.");
	return OBJ_VAL((ObjInstance*) turnInstance);
}

Value arc(Value* params, int numParams){
	ObjShape* arcInstance = allocateShape(NULL, TK_ARC);
	add(arcInstance->instance.map, string("center"), VECTOR(0, 0));
	add(arcInstance->instance.map, string("degrees"), NUM_VAL(0));

	ObjInstance* current = currentInstance();
	if(current && current->type == INST_SHAPE){
		ObjShape* shape = (ObjShape*) current;
		if(shape->shapeType == TK_PATH){
			addSegment(shape, arcInstance);
			return OBJ_VAL((ObjInstance*) arcInstance);
		}
	}

	runtimeError("Only paths can accept arcs.");
	return OBJ_VAL((ObjInstance*) arcInstance);
}

Value rect(Value* params, int numParams){
	ObjShape* rectInstance = allocateShape(NULL, TK_RECT);
	add(rectInstance->instance.map, string("position"), VECTOR(0, 0));
	add(rectInstance->instance.map, string("size"), VECTOR(0, 0));
	add(rectInstance->instance.map, string("rounding"), NUM_VAL(0));
	pushShape(rectInstance);
	return OBJ_VAL((ObjInstance*) rectInstance);
}

Value circle(Value* params, int numParams){
	ObjShape* circInstance = allocateShape(NULL, TK_CIRCLE);
	add(circInstance->instance.map, string("position"), VECTOR(0, 0));
	add(circInstance->instance.map, string("radius"), NUM_VAL(0));
	pushShape(circInstance);
	return OBJ_VAL((ObjInstance*) circInstance);
}

Value ellipse(Value* params, int numParams){
	ObjShape* ellipseInstance = allocateShape(NULL, TK_ELLIPSE);
	add(ellipseInstance->instance.map, string("radius"), VECTOR(0, 0));
	add(ellipseInstance->instance.map, string("position"), VECTOR(0, 0));
	pushShape(ellipseInstance);
	return OBJ_VAL((ObjInstance*) ellipseInstance);
}

Value line(Value* params, int numParams){
	ObjShape* lineInstance = allocateShape(NULL, TK_LINE);
	add(lineInstance->instance.map, string("start"), VECTOR(0, 0));
	add(lineInstance->instance.map, string("end"), VECTOR(0, 0));
	pushShape(lineInstance);
	return OBJ_VAL((ObjInstance*) lineInstance);
}

Value polyline(Value* params, int numParams){
	ObjShape* polylInstance = allocateShape(NULL, TK_POLYLINE);
	pushShape(polylInstance);

	return OBJ_VAL((ObjInstance*) polylInstance);
}

Value polygon(Value* params, int numParams){
	ObjShape* polygInstance = allocateShape(NULL, TK_POLYGON);
	pushShape(polygInstance);

	return OBJ_VAL((ObjInstance*) polygInstance);
}

Value path(Value* params, int numParams){
	ObjShape* pathInstance = allocateShape(NULL, TK_PATH);
	add(pathInstance->instance.map, string("closed"), BOOL_VAL(true));
	pushShape(pathInstance);

	return OBJ_VAL((ObjInstance*) pathInstance);
}

Value ungon(Value* params, int numParams){
	ObjShape* ungonInstance = allocateShape(NULL, TK_UNGON);
	add(ungonInstance->instance.map, string("closed"), BOOL_VAL(true));
	pushShape(ungonInstance);

	return OBJ_VAL((ObjInstance*) ungonInstance);
}

Value qBezier(Value* params, int numParams){
	ObjShape* bezInstance = allocateShape(NULL, TK_QBEZIER);
	add(bezInstance->instance.map, string("control"), VECTOR(0, 0));
	add(bezInstance->instance.map, string("end"), VECTOR(0, 0));

	ObjInstance* current = currentInstance();
	if(current && current->type == INST_SHAPE){
		ObjShape* shape = (ObjShape*) current;
		if(shape->shapeType == TK_PATH){
			addSegment(shape, bezInstance);
			return OBJ_VAL((ObjInstance*) bezInstance);
		}
	}

	runtimeError("Only paths can accept Beziers.");
	return OBJ_VAL((ObjInstance*) bezInstance);
}

Value cBezier(Value* params, int numParams){
	ObjShape* bezInstance = allocateShape(NULL, TK_CBEZIER);

	add(bezInstance->instance.map, string("startControl"), VECTOR(0, 0));
	add(bezInstance->instance.map, string("endControl"), VECTOR(0, 0));
	add(bezInstance->instance.map, string("end"), VECTOR(0, 0));
	
	ObjInstance* current = currentInstance();
	if(current && current->type == INST_SHAPE){
		ObjShape* shape = (ObjShape*) current;
		if(shape->shapeType == TK_PATH){
			addSegment(shape, bezInstance);
			return OBJ_VAL((ObjInstance*) bezInstance);
		}
	}

	runtimeError("Only paths can accept Beziers.");
	return OBJ_VAL((ObjInstance*) bezInstance);
}

Value mirror(Value* params, int numParams){
	ObjShape* mirrorInstance = allocateShape(NULL, TK_MIRROR);
	
	add(mirrorInstance->instance.map, string("axis"), NUM_VAL((double) CS_X));
	add(mirrorInstance->instance.map, string("origin"), VECTOR(0, 0));

	ObjInstance* current = currentInstance();
	if(current && current->type == INST_SHAPE){
		ObjShape* shape = (ObjShape*) current;
		if(POLY_COMPATIBLE(shape)){
			addSegment(shape, mirrorInstance);
			return OBJ_VAL((ObjInstance*) mirrorInstance);
		}
	}

	runtimeError("Only poly-type shapes or paths can be mirrored.");
	return OBJ_VAL((ObjInstance*) mirrorInstance);
}

Value nativeTranslate(Value* params, int numParams){
	ObjShape* translate = allocateShape(NULL, TK_CBEZIER);

	add(translate->instance.map, string("diff"), VECTOR(0, 0));
	
	ObjInstance* current = currentInstance();
	if(current && current->type == INST_SHAPE){
		ObjShape* shape = (ObjShape*) current;
		if(shape->shapeType == TK_PATH){
			addSegment(shape, translate);
			return OBJ_VAL((ObjInstance*) translate);
		}
	}

	runtimeError("Only paths can accept beziers.");
	return OBJ_VAL((ObjInstance*) translate);
}

void initGlobals(HashMap* map){
	ObjString* canvasString = string("canvas");
	ObjString* originString = string("origin");
	ObjString* sizeString = string("size");
	ObjInstance* canvasProperties = allocateInstance(NULL);
	add(canvasProperties->map, originString, VECTOR(0, 0));
	add(canvasProperties->map, sizeString, VECTOR(500, 500));
	add(map, canvasString, OBJ_VAL(canvasProperties));
}