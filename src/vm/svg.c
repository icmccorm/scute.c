#include <stdio.h>
#include <stdlib.h>
#include "svg.h"
#include "hashmap.h"
#include "obj.h"
#include "value.h"
#include "vm.h"
#include "scanner.h"

void drawShape(Shape* shape){
		#ifdef EM_MAIN
			#define ATTR(name, value) (addAttribute(name, AS_NUM(value), value.charIndex));

			unsigned address = (unsigned) shape;
			newShape(address, shape->shapeType);
			switch(shape->shapeType){
				case TK_RECT: {
					Rect* rect = AS_RECT(shape);
					ATTR("x", rect->x);
					ATTR("y", rect->y);
					ATTR("width", rect->w);
					ATTR("height", rect->h);
				} break;
				default:
					break;
			}
			paintShape();
		#else
			printValue(O_OUT, OBJ_VAL(shape, -1));
		#endif
}

void renderFrame(Shape* shape){
	while(shape != NULL){
		drawShape(shape);
		shape = shape->next;
	}
}

void assignPosition(ObjClosure* close, Value* values, uint8_t numValues){
	Shape* shape = close->shape;
	switch(shape->shapeType){
		case TK_RECT: {
			Rect* rect = AS_RECT(shape);
			if(numValues == 1){
				rect->x = values[0];
				rect->y = values[0];
			}else{
				rect->x = values[0];
				rect->y = values[1];	
			}
		} break;
		default:
			break;
	}

}
void assignDimensions(ObjClosure* close, Value* values, uint8_t numValues){
	Shape* shape = close->shape;
	switch(shape->shapeType){
		case TK_RECT: {
			Rect* rect = AS_RECT(shape);
			if(numValues == 1){
				rect->w = values[0];
				rect->h = values[0];
			}else{
				rect->w = values[0];
				rect->h = values[1];	
			}
		} break;
		default:
			break;
	}
}