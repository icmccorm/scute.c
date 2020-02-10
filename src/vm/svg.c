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
#include "math.h"

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

void drawPoints(ObjShape* shape){
	unsigned address = (unsigned) shape->instance.map;
	#ifdef EM_MAIN
		newShape(address, shape->shapeType);
	#endif
	
	double angle = 0;
	int prevPoint[2];
	prevPoint[0] = 0;
	prevPoint[1] = 0;

	for(int i = 0; i<shape->numSegments; ++i){
		ObjShape* segment = shape->segments[i];
		HashMap* map = segment->instance.map;

		switch(segment->shapeType){
			case TK_JUMP: ;
				ObjArray* vector = AS_ARRAY(getValue(map, string("position")));
				prevPoint[0] = (int) AS_NUM(vector->array->values[0]);
				prevPoint[1] = (int) AS_NUM(vector->array->values[1]);
				#ifdef EM_MAIN
					em_addPoint(prevPoint);
				#else
					print(O_OUT, "%d, %d\n", prevPoint[0], prevPoint[1]);
				#endif
				break;
			case TK_MOVE: ;
				double distance = AS_NUM(getValue(map, string("distance")));
				prevPoint[0] += (int) round(cos(angle) * distance);
				prevPoint[1] += (int) round(sin(angle) * distance);
				#ifdef EM_MAIN
					em_addPoint(prevPoint);
				#else
					print(O_OUT, "%d, %d\n", prevPoint[0], prevPoint[1]);
				#endif
				break;
			case TK_TURN:
				angle += AS_NUM(getValue(map, string("degrees")));
				break;
			default:
				break;
		}
	}
	#ifdef EM_MAIN
		paintShape();
	#endif
}


void drawPath(ObjShape* shape){	
	unsigned address = (unsigned) shape->instance.map;
	#ifdef EM_MAIN
		newShape(address, shape->shapeType);
	#endif
	int angle = 0;
	int prevPoint[2];
	prevPoint[0] = 0;
	prevPoint[1] = 0;

	for(int i = 0; i<shape->numSegments; ++i){
		ObjShape* segment = shape->segments[i];
		switch(segment->shapeType){
			case TK_JUMP:
				break;
			case TK_MOVE:
				break;
			case TK_TURN:
				break;
			case TK_ARC:
				break;
			default:
				break;
		}
	}
	#ifdef EM_MAIN
		paintShape();
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
		switch(top->shapeType){
			case TK_POLY:
			case TK_POLYL:
				drawPoints(top);
				break;
			case TK_PATH:
				drawPath(top);
			default:
				drawShape(top->instance.map, top->shapeType);	
				break;
		}
	}
}