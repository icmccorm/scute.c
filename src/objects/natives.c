#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include "value.h"
#include "common.h"
#include "vm.h"
#include "natives.h"
#include "obj.h"

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
		return NUM_VAL(number * 180 / PI);
	}else{
		runtimeError("Missing operand.");
		return NULL_VAL();
	}
}

Value nativeRadians(Value* params, int numParams){
	if(numParams > 0) {
		Value operand = params[0];
		double number = AS_NUM(operand);
		return NUM_VAL(number * PI / 180);
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
	ObjInstance* current = currentInstance();
	if(current && current->type == INST_SHAPE){
		ObjShape* shape = (ObjShape*) current;
		if(shape->shapeType >= TK_POLY){
			ObjShape* jumpInstance = allocateShape(NULL, TK_JUMP);
			add(jumpInstance->instance.map, string("position"), VECTOR(0, 0));

			addSegment(shape, jumpInstance);
			return OBJ_VAL((ObjInstance*) jumpInstance);
		}
	}
	runtimeError("Only polylines, polygons, and paths can accept move commands.");
	return NULL_VAL();
}

Value move(Value* params, int numParams){

	ObjShape* moveInstance = allocateShape(NULL, TK_MOVE);
	add(moveInstance->instance.map, string("distance"), NUM_VAL(0));

	ObjInstance* current = currentInstance();
	if(current && current->type == INST_SHAPE){
		ObjShape* shape = (ObjShape*) current;
		if(shape->shapeType >= TK_POLY){
			addSegment(shape, moveInstance);
			return OBJ_VAL((ObjInstance*) moveInstance);
		}
	}
	runtimeError("Only polylines, polygons, and paths can accept move commands.");
	return OBJ_VAL((ObjInstance*) moveInstance);
}

Value vertex(Value* params, int numParams) {
	ObjShape* vertexInstance = allocateShape(NULL, TK_VERT);
	add(vertexInstance->instance.map, string("position"), VECTOR(0, 0));

	ObjInstance* current = currentInstance();
	if(current && current->type == INST_SHAPE){
		ObjShape* shape = (ObjShape*) current;
		if(shape->shapeType >= TK_POLY){
			addSegment(shape, vertexInstance);
			return OBJ_VAL((ObjInstance*) vertexInstance);
		}
	}
	runtimeError("Only polylines, polygons, and paths can accept vertices.");
	return OBJ_VAL((ObjInstance*) vertexInstance);
}

Value turn(Value* params, int numParams){
	ObjShape* turnInstance = allocateShape(NULL, TK_TURN);
	add(turnInstance->instance.map, string("degrees"), NUM_VAL(0));

	ObjInstance* current = currentInstance();
	if(current && current->type == INST_SHAPE){
		ObjShape* shape = (ObjShape*) current;
		if(shape->shapeType >= TK_POLY){
			addSegment(shape, turnInstance);
			return OBJ_VAL((ObjInstance*) turnInstance);
		}
	}
	runtimeError("Only polylines, polygons, and paths can accept turn commands.");
	return OBJ_VAL((ObjInstance*) turnInstance);
}

Value arc(Value* params, int numParams){
	ObjShape* arcInstance = allocateShape(NULL, TK_TURN);
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
	ObjShape* rectInstance = allocateShape(NULL, TK_TURN);
	add(rectInstance->instance.map, string("position"), VECTOR(0, 0));
	add(rectInstance->instance.map, string("dimensions"), NUM_VAL(0));

	return OBJ_VAL((ObjInstance*) rectInstance);
}

Value circle(Value* params, int numParams){
	ObjShape* circInstance = allocateShape(NULL, TK_TURN);
	add(circInstance->instance.map, string("position"), VECTOR(0, 0));
	add(circInstance->instance.map, string("radius"), NUM_VAL(0));

	return OBJ_VAL((ObjInstance*) circInstance);
}

Value ellipse(Value* params, int numParams){
	ObjShape* rectInstance = allocateShape(NULL, TK_TURN);
	add(rectInstance->instance.map, string("radius"), VECTOR(0, 0));
	add(rectInstance->instance.map, string("position"), VECTOR(0, 0));

	return OBJ_VAL((ObjInstance*) rectInstance);
}

Value polyline(Value* params, int numParams){
	ObjShape* polylInstance = allocateShape(NULL, TK_TURN);
	
	return OBJ_VAL((ObjInstance*) polylInstance);
}

Value polygon(Value* params, int numParams){
	ObjShape* polygInstance = allocateShape(NULL, TK_TURN);
	
	return OBJ_VAL((ObjInstance*) polygInstance);
}

Value path(Value* params, int numParams){
	ObjShape* pathInstance = allocateShape(NULL, TK_TURN);
	
	return OBJ_VAL((ObjInstance*) pathInstance);
}
