#ifndef scute_natives_h
#define scute_natives_h

#include "value.h"
typedef Value (*NativeFn)(Value* params, int numParams);

Value nativeHyptan(Value* params, int numParams);
Value nativeHypcos(Value* params, int numParams);
Value nativeHypsin(Value* params, int numParams);
Value nativeArctan(Value* params, int numParams);
Value nativeArccos(Value* params, int numParams);
Value nativeArcsin(Value* params, int numParams);
Value nativeCosine(Value* params, int numParams);
Value nativeTangent(Value* params, int numParams);
Value nativeSine(Value* params, int numParams);
Value nativeDegrees(Value* params, int numParams);
Value nativeRadians(Value* params, int numParams);
Value nativeSqrt(Value* params, int numParams);

Value move(Value* params, int numParams);
Value vertex(Value* params, int numParams);
Value turn(Value* params, int numParams);
Value arc(Value* params, int numParams);
Value jump(Value* params, int numParams);

#endif