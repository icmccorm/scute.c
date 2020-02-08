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

		STYLE("strokeWidth", getValue(shapeMap, string("strokeWidth")));
		addStyle("strokeWidth", &strokeWidth);

		Value fill = getValue(shapeMap, string("fill"));
		resolveColor("fill", fill);

		Value stroke = getValue(shapeMap, string("stroke"));
		resolveColor("stroke", stroke);

		switch(type){
			case TK_RECT: { 
				ObjString* xStr = string("x");
				ObjString* yStr = string("y");
				ObjString* wStr = string("width");
				ObjString* hStr = string("height");

				Value xVal = getValue(shapeMap, xStr);
				addAttribute("x", &xVal);

				Value yVal = getValue(shapeMap, yStr);
				addAttribute("y", &yVal);

				Value wVal = getValue(shapeMap, wStr);
				addAttribute("width", &wVal);

				Value hVal = getValue(shapeMap, hStr);
				addAttribute("height", &hVal);
			} break;

			case TK_CIRC:{
				ObjString* cxStr = string("cx");
				ObjString* cyStr = string("cy");
				ObjString* rStr = string("r");

				Value cxVal = getValue(shapeMap, cxStr);
				addAttribute("cx", &cxVal);

				Value cyVal = getValue(shapeMap, cyStr);
				addAttribute("cy", &cyVal);

				Value rVal = getValue(shapeMap, rStr);
				addAttribute("r", &rVal);
				} break;
			default:
				break;
		}
		paintShape();
	#else
		printMap(O_OUT, shapeMap, 0);
	#endif
}

ObjShape* popShape(){
	ObjShape* latestShape = vm.shapeStack[vm.shapeCount-1];
	--vm.shapeCount;
	return latestShape;
}

void pushShape(ObjShape* close){
	if(vm.shapeCount + 1 > vm.shapeCapacity){
		int oldCapacity = vm.shapeCapacity;
		vm.shapeCapacity = GROW_CAPACITY(oldCapacity);
		vm.shapeStack = GROW_ARRAY(vm.shapeStack, ObjShape*, oldCapacity, vm.shapeCapacity);
	}
	vm.shapeStack[vm.shapeCount] = close;
	++vm.shapeCount;
}

void renderFrame(){
	while(vm.shapeCount > 0){
		ObjShape* top = popShape();
		drawShape(top->map, top->shapeType);
	}
}