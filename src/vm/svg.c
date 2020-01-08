#include <stdio.h>
#include <stdlib.h>
#include "svg.h"
#include "hashmap.h"
#include "obj.h"
#include "obj_def.h"
#include "value.h"
#include "vm.h"
#include "scanner.h"

void drawShape(HashMap* shapeMap, TKType type){
	#ifdef EM_MAIN
		#define ATTR(name, value) (addAttribute(name, AS_NUM(value), value.charIndex, value.line));
		unsigned address = (unsigned) shapeMap;
		newShape(address, type);

			Value stroke = getValue(shapeMap, internString("stroke", 6));
			switch(stroke.type){
				case VL_OBJ: {
					Obj* strokeObj = AS_OBJ(stroke);
					if(strokeObj->type == OBJ_INST){
						HashMap* strokeMap = ((ObjInstance*) strokeObj)->map;
						Value width = getValue(strokeMap, internString("width", 5));
						Value color = getValue(strokeMap, internString("color", 5));
						ATTR("stroke", color);
						ATTR("stroke-width", width);
					}
					} break;
				default:
					ATTR("stroke", stroke);
					break;
			}

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
				ATTR("r", wVal);

				} break;
			default:
				break;
		}
		paintShape();
	#else
		printMap(O_OUT, shapeMap, 0);
	#endif
}

void renderFrame(ObjInstance* close){
	ObjInstance* current = close;
	while(current != NULL){
		drawShape(current->map, current->instanceType);
		current = current->nextShape;
	}
}