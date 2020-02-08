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
	if(current->object.type == OBJ_INST_SHAPE){
		ObjShape* shape = (ObjShape*) current;
		if(shape->shapeType >= TK_POLY){
			ObjShape* jumpInstance = allocateShape(NULL, TK_JUMP);
			add(jumpInstance->map, string("position"), VECTOR(0, 0));

			addSegment(shape, jumpInstance);
			return OBJ_VAL((ObjInstance*) jumpInstance);
		}
	}
	runtimeError("Only polylines, polygons, and paths can accept move commands.");
	return NULL_VAL();
}

Value move(Value* params, int numParams){
	ObjInstance* current = currentInstance();
	if(current->object.type == OBJ_INST_SHAPE){
		ObjShape* shape = (ObjShape*) current;
		if(shape->shapeType >= TK_POLY){
			ObjShape* moveInstance = allocateShape(NULL, TK_MOVE);
			add(moveInstance->map, string("distance"), NUM_VAL(0));

			addSegment(shape, moveInstance);
			return OBJ_VAL((ObjInstance*) moveInstance);
		}
	}
	runtimeError("Only polylines, polygons, and paths can accept move commands.");
	return NULL_VAL();

}

Value vertex(Value* params, int numParams) {
	ObjInstance* current = currentInstance();
	if(current->object.type == OBJ_INST_SHAPE){
		ObjShape* shape = (ObjShape*) current;
		if(shape->shapeType >= TK_POLY){
			ObjShape* vertexInstance = allocateShape(NULL, TK_VERT);
			add(vertexInstance->map, string("position"), VECTOR(0, 0));
			addSegment(shape, vertexInstance);
			return OBJ_VAL((ObjInstance*) vertexInstance);
		}
	}
	runtimeError("Only polylines, polygons, and paths can accept vertices.");
	return NULL_VAL();

}

Value turn(Value* params, int numParams){
	ObjInstance* current = currentInstance();
	if(current->object.type == OBJ_INST_SHAPE){
		ObjShape* shape = (ObjShape*) current;
		if(shape->shapeType >= TK_POLY){
			ObjShape* turnInstance = allocateShape(NULL, TK_TURN);
			add(turnInstance->map, string("position"), VECTOR(0, 0));
			addSegment(shape, turnInstance);
			return OBJ_VAL((ObjInstance*) turnInstance);
		}
	}
	runtimeError("Only polylines, polygons, and paths can accept turn commands.");
	return NULL_VAL();

}

Value arc(Value* params, int numParams){
	ObjInstance* current = currentInstance();
	if(current->object.type == OBJ_INST_SHAPE){
		ObjShape* shape = (ObjShape*) current;
		if(shape->shapeType == TK_PATH){
			ObjShape* arcInstance = allocateShape(NULL, TK_TURN);
			add(arcInstance->map, string("center"), VECTOR(0, 0));
			add(arcInstance->map, string("degrees"), NUM_VAL(0));

			addSegment(shape, arcInstance);
			return OBJ_VAL((ObjInstance*) arcInstance);
		}
	}
	runtimeError("Only paths can accept arcs.");
	return NULL_VAL();
}

