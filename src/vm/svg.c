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
					if(strokeObj->type == OBJ_SCOPE){
						HashMap* strokeMap = ((ObjScope*) strokeObj)->map;
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

			Value fill = getValue(shapeMap, internString("fill", 4));
			switch(fill.type){
				case VL_OBJ: { 
						ObjString* xStr = internString("x", 1);
						ObjString* yStr = internString("y", 1);
						ObjString* wStr = internString("width", 5);
						ObjString* hStr = internString("height", 6);

						Value xVal = getValue(shapeMap, xStr);
						ATTR("x", xVal);

						Value yVal = getValue(shapeMap, yStr);
						ATTR("y", yVal);

						Value wVal = getValue(shapeMap, wStr);
						ATTR("w", wVal);

						Value hVal = getValue(shapeMap, hStr);
						ATTR("h", hVal);

					} break;
				default:
					break;
			}
			paintShape();
		#else
			printMap(O_OUT, shapeMap, 0);
		#endif
}

void renderFrame(ObjScope* close){
	ObjScope* current = close;
	while(current != NULL){
		drawShape(current->map, current->shapeType);
		current = current->nextShape;
	}
}