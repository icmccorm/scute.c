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