#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include "value.h"
#include "common.h"
#include "vm.h"
#include "natives.h"
#include "obj.h"
#include "svg.h"

#define IS_SHAPE(current) ((current && current->type == INST_SHAPE))
#define PATH(current) (IS_SHAPE(current) && ((ObjShape*)current)->shapeType == TK_PATH)
#define POLYPATH(current) (IS_SHAPE(current) && ((ObjShape*)current)->shapeType >= TK_POLY)

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
	ObjInstance* jumpInstance = allocateInstance(NULL, INST_SEG, TK_JUMP);
	add(jumpInstance->map, string("position"), VECTOR(0, 0));

	ObjInstance* current = latestInstance();
	if(POLYPATH(current)){
		ObjShape* shape = (ObjShape*) current;
		addSegment(shape, jumpInstance);
		return OBJ_VAL(jumpInstance);
	}
	
	runtimeError("Only polylines, polygons, and paths can accept move commands.");
	return OBJ_VAL(jumpInstance);
}

Value move(Value* params, int numParams){

	ObjInstance* moveInstance = allocateInstance(NULL, INST_SEG, TK_MOVE);
	add(moveInstance->map, string("distance"), NUM_VAL(0));

	ObjInstance* current = latestInstance();
	if(POLYPATH(current)){
		ObjShape* shape = (ObjShape*) current;
		addSegment(shape, moveInstance);
		return OBJ_VAL(moveInstance);
	}
	
	runtimeError("Only polylines, polygons, and paths can accept move commands.");
	return OBJ_VAL(moveInstance);
}

Value vertex(Value* params, int numParams) {
	ObjInstance* vertexInstance = allocateInstance(NULL, INST_SEG, TK_VERT);
	add(vertexInstance->map, string("position"), VECTOR(0, 0));

	ObjInstance* current = latestInstance();
	if(POLYPATH(current)){
		ObjShape* shape = (ObjShape*) current;
		addSegment(shape, vertexInstance);
		return OBJ_VAL(vertexInstance);
	}
	
	runtimeError("Only polylines, polygons, and paths can accept vertices.");
	return OBJ_VAL(vertexInstance);
}

Value turn(Value* params, int numParams){
	ObjInstance* turnInstance = allocateInstance(NULL, INST_SEG, TK_TURN);
	add(turnInstance->map, string("degrees"), NUM_VAL(0));
	ObjInstance* current = latestInstance();

	if(POLYPATH(current)){
		ObjShape* shape = (ObjShape*) current;
		addSegment(shape, turnInstance);
		return OBJ_VAL(turnInstance);
	}
	
	runtimeError("Only polylines, polygons, and paths can accept turn commands.");
	return OBJ_VAL(turnInstance);
}

Value arc(Value* params, int numParams){
	ObjInstance* arcInstance = allocateInstance(NULL, INST_SEG, TK_ARC);
	add(arcInstance->map, string("center"), VECTOR(0, 0));
	add(arcInstance->map, string("radius"), VECTOR(0, 0));
	add(arcInstance->map, string("degrees"), VECTOR(0, 0));

	ObjInstance* current = latestInstance();
	if(PATH(current)){
		ObjShape* shape = (ObjShape*) current;
		addSegment(shape, arcInstance);
		return OBJ_VAL(arcInstance);
	}

	runtimeError("Only paths can accept arcs.");
	return OBJ_VAL(arcInstance);
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
	ObjShape* circInstance = allocateShape(NULL, TK_CIRC);
	add(circInstance->instance.map, string("position"), VECTOR(0, 0));
	add(circInstance->instance.map, string("radius"), NUM_VAL(0));
	pushShape(circInstance);
	return OBJ_VAL((ObjInstance*) circInstance);
}

Value ellipse(Value* params, int numParams){
	ObjShape* ellipseInstance = allocateShape(NULL, TK_ELLIP);
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
	ObjShape* polylInstance = allocateShape(NULL, TK_POLYL);
	pushShape(polylInstance);

	return OBJ_VAL((ObjInstance*) polylInstance);
}

Value polygon(Value* params, int numParams){
	ObjShape* polygInstance = allocateShape(NULL, TK_POLY);
	pushShape(polygInstance);

	return OBJ_VAL((ObjInstance*) polygInstance);
}

Value path(Value* params, int numParams){
	ObjShape* pathInstance = allocateShape(NULL, TK_PATH);
	pushShape(pathInstance);

	return OBJ_VAL((ObjInstance*) pathInstance);
}

Value qBezier(Value* params, int numParams){
	ObjInstance* bezInstance = allocateInstance(NULL, INST_SEG, TK_QBEZ);
	add(bezInstance->map, string("control"), VECTOR(0, 0));
	add(bezInstance->map, string("end"), VECTOR(0, 0));

	ObjInstance* current = latestInstance();
		
	if(POLYPATH(current)){
		ObjShape* shape = (ObjShape*) current;
		addSegment(shape, bezInstance);
		return OBJ_VAL((ObjInstance*) bezInstance);
	}

	runtimeError("Only paths can accept beziers.");
	return OBJ_VAL((ObjInstance*) bezInstance);
}

Value translate(Value* params, int numParams){
	ObjInstance* transInstance = allocateInstance(NULL, INST_TRANS, TK_CBEZ);
	add(transInstance->map, string("vector"), VECTOR(0, 0));

	ObjInstance* current = latestInstance();
	if(IS_SHAPE(current)){
		ObjShape* shape = (ObjShape*) current;
		addTransform(shape, transInstance);
		return OBJ_VAL((ObjInstance*) transInstance);
	}
	runtimeError("Only paths can accept beziers.");
	return OBJ_VAL((ObjInstance*) transInstance);
}

Value rotate(Value* params, int numParams){
	return NULL_VAL();
}

Value skew(Value* params, int numParams){
	return NULL_VAL();

}

Value matrix(Value* params, int numParams){
	return NULL_VAL();

}

Value scale(Value* params, int numParams){
	return NULL_VAL();

}

Value cBezier(Value* params, int numParams){
	ObjInstance* bezInstance = allocateInstance(NULL, INST_SEG, TK_CBEZ);
	add(bezInstance->map, string("startControl"), VECTOR(0, 0));
	add(bezInstance->map, string("endControl"), VECTOR(0, 0));
	add(bezInstance->map, string("end"), VECTOR(0, 0));
	
	ObjInstance* current = latestInstance();
	if(PATH(current)){
		ObjShape* shape = (ObjShape*) current;
		addSegment(shape, bezInstance);
		return OBJ_VAL(bezInstance);
	}

	runtimeError("Only paths can accept beziers.");
	return OBJ_VAL(bezInstance);
}