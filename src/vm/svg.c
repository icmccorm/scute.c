#include <stdio.h>
#include <stdlib.h>
#include "svg.h"
#include "hashmap.h"
#include "obj.h"
#include "obj_def.h"
#include "value.h"
#include "vm.h"
#include "scanner.h"
#include "color.h"

#ifdef EM_MAIN
void resolveColor(const char* key, Value val){
	Color* color = NULL;
	if(IS_OBJ(val)){
		Obj* objVal = AS_OBJ(val);
		switch(objVal->type){
			case OBJ_COLOR: ;
				ObjColor* colorObj = AS_COLOR(val);
				color = colorObj->color;
				addColorAttribute(key, color);
				break;
			default:
				break;
		}
	}
}
#endif

void drawShape(HashMap* shapeMap, TKType type){
	#ifdef EM_MAIN
		unsigned address = (unsigned) shapeMap;
		newShape(address, type);

		Value stroke = getValue(shapeMap, internString("stroke", 6));
		STYLE("strokeWidth", getValue(shapeMap, internString("strokeWidth", 11)));

		Value fill = getValue(shapeMap, internString("fill", 6));
		resolveColor("fill", fill);

		switch(type){
			case TK_RECT: { 
				ObjString* xStr = internString("x", 1);
				ObjString* yStr = internString("y", 1);
				ObjString* wStr = internString("width", 5);
				ObjString* hStr = internString("height", 6);

				Value xVal = getValue(shapeMap, xStr);
				ATTR("x", xVal);

				Value yVal = getValue(shapeMap, yStr);
				ATTR("y", yVal);

				Value wVal = getValue(shapeMap, wStr);
				ATTR("width", wVal);

				Value hVal = getValue(shapeMap, hStr);
				ATTR("height", hVal);
			} break;

			case TK_CIRC:{
				ObjString* cxStr = internString("cx", 2);
				ObjString* cyStr = internString("cy", 2);
				ObjString* rStr = internString("r", 1);

				Value cxVal = getValue(shapeMap, cxStr);
				ATTR("cx", cxVal);

				Value cyVal = getValue(shapeMap, cyStr);
				ATTR("cy", cyVal);

				Value rVal = getValue(shapeMap, rStr);
				ATTR("r", rVal);

				} break;
			default:
				break;
		}
		paintShape();
	#else
		printMap(O_OUT, shapeMap, 0);
	#endif
}


ObjInstance* popShape(){
	--vm.shapeCount;
	--vm.shapeStackTop;
	return *vm.shapeStackTop;
}

void pushShape(ObjInstance* close){
	if(vm.shapeCount + 1 > vm.shapeCapacity){

		int oldCapacity = vm.shapeCapacity;
		vm.shapeCapacity = GROW_CAPACITY(oldCapacity);
		vm.shapeStack = GROW_ARRAY(vm.shapeStack, ObjInstance*, oldCapacity, vm.shapeCapacity);
		if(oldCapacity == 0) vm.shapeStackTop = vm.shapeStack;
	}
	vm.shapeStack[vm.shapeCount] = close;
	++vm.shapeStackTop;
	++vm.shapeCount;
}

void renderFrame(){
	while(vm.shapeCount > 0){
		ObjInstance* top = popShape();
		drawShape(top->map, top->instanceType);
	}
}