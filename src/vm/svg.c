#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "svg.h"
#include "hashmap.h"
#include "obj.h"
#include "obj_def.h"
#include "value.h"
#include "vm.h"
#include "scanner.h"
#include "color.h"
#include "package.h"

#ifdef EM_MAIN

void resolveColor(const char* key, Value val){
	if(IS_OBJ(val)){
		Obj* objVal = AS_OBJ(val);
		switch(objVal->type){
			case OBJ_ARRAY: ;
		 		ValueArray* vArray = AS_ARRAY(val)->array;
				em_addColorStyle(key, vArray->count, vArray->values);
				break;
			default:
				break;
		}
	}else{
		if(IS_NUM(val)){
			
		}
	}
}

void assignStyles(ObjShape* shape){
	HashMap* shapeMap = shape->instance.map;
	Value strokeWidth = getValue(shapeMap, string("strokeWidth"));
	em_addStyle("strokeWidth", &strokeWidth);

	Value fill = getValue(shapeMap, string("fill"));
	resolveColor("fill", fill);

	Value stroke = getValue(shapeMap, string("stroke"));
	resolveColor("stroke", stroke);
}
#endif

void drawShape(ObjShape* shape){
	HashMap* shapeMap = shape->instance.map;
	TKType type = shape->shapeType;
	#ifdef EM_MAIN
		unsigned address = (unsigned) shapeMap;
		em_newShape(address, type);
		
		assignStyles(shape);

		switch(type){
			case TK_RECT: { 
				Value posVal = getValue(shapeMap, string("position"));
				Value sizeVal = getValue(shapeMap, string("size"));
				Value roundingVal = getValue(shapeMap, string("rounding"));

				ValueArray* sizeArray = AS_ARRAY(sizeVal)->array;
				ValueArray* posArray = AS_ARRAY(posVal)->array;
				
				em_addVectorAttr("size", sizeArray->values);
				em_addVectorAttr("position", posArray->values);
				em_addAttribute("rounding", &roundingVal);
			} break;

			case TK_CIRC:{
				Value posVal = getValue(shapeMap, string("position"));
				Value radVal = getValue(shapeMap, string("radius"));
				ValueArray* posArray = AS_ARRAY(posVal)->array;
				
				em_addVectorAttr("position", posArray->values);
				em_addAttribute("radius", &radVal);
				} break;

			case TK_ELLIP: {
				Value posVal = getValue(shapeMap, string("position"));
				Value radVal = getValue(shapeMap, string("radius"));

				ValueArray* posArray = AS_ARRAY(posVal)->array;
				ValueArray* radArray = AS_ARRAY(radVal)->array;

				em_addVectorAttr("position", posArray->values);
				em_addVectorAttr("radius", posArray->values);
				} break;

			case TK_LINE: {
				Value startVal = getValue(shapeMap, string("start"));
				Value endVal = getValue(shapeMap, string("end"));

				ValueArray* startArray = AS_ARRAY(startVal)->array;
				ValueArray* endArray = AS_ARRAY(endVal)->array;

				em_addVectorAttr("start", startArray->values);
				em_addVectorAttr("end", endArray->values);
				} break;
			
			default:
				break;
		}
		em_paintShape();
		#else	
			printMap(O_OUT, shapeMap, 0);
		#endif
}

double toRadians(int degrees){
	return (3.14/180) * degrees;
}

void drawPoints(ObjShape* shape){
	unsigned address = (unsigned) shape->instance.map;
	#ifdef EM_MAIN
		em_newShape(address, shape->shapeType);
		assignStyles(shape);
	#endif

	int angle = 0;
	int points[2];
	points[0] = 0;
	points[1] = 1;

	for(int i = 0; i<shape->numSegments; ++i){
		ObjShape* segment = shape->segments[i];
		HashMap* map = segment->instance.map;

		switch(segment->shapeType){
			case TK_JUMP: {
				ObjArray* vector = AS_ARRAY(getValue(map, string("position")));
				points[0] = AS_NUM(getValueArray(vector->array, 0));
				points[1] = AS_NUM(getValueArray(vector->array, 1));
				#ifdef EM_MAIN
					em_addJump(vector->array->values);
				#else
					print(O_OUT, "Jump: (%d, %d)\n", points[0], points[1]);
				#endif
			} break;
			case TK_VERT: ;
				ObjArray* vector = AS_ARRAY(getValue(map, string("position")));
				points[0] = AS_NUM(getValueArray(vector->array, 0));
				points[1] = AS_NUM(getValueArray(vector->array, 1));
				#ifdef EM_MAIN
					em_addVertex(vector->array->values);
				#else
					print(O_OUT, "Vertex: (%d, %d)\n", points[0], points[1]);
				#endif
				break;
			case TK_MOVE: ;
				Value distance = getValue(map, string("distance"));
				points[0] += (int) round(cos(toRadians(angle))*AS_NUM(distance));
				points[1] += (int) round(sin(toRadians(angle))*AS_NUM(distance));
				#ifdef EM_MAIN
					em_addMove(&distance);				
				#else
					print(O_OUT, "Move %f: (%d, %d)\n", AS_NUM(distance), points[0], points[1]);
				#endif
				break;
			case TK_TURN: ;
				Value degrees = getValue(map, string("degrees"));
				#ifdef EM_MAIN
					em_addTurn(&degrees);				
				#else
					print(O_OUT, "Turn by %d deg to %d deg\n", degrees, angle);
				#endif
				angle += AS_NUM(degrees);
				break;
			case TK_QBEZ: {
				Value control = getValue(map, string("control"));
				Value end = getValue(map, string("end"));

				Value* controlArray = AS_ARRAY(control)->array->values;
				Value* endArray = AS_ARRAY(end)->array->values;
				#ifdef EM_MAIN
					em_addQuadBezier(controlArray, endArray);
				#else
					print(O_OUT, "qBezier ");
					printValue(O_OUT, control);
					print(O_OUT, " ");
					printValue(O_OUT, end);
					print(O_OUT, "\n");
				#endif
			} break;
			case TK_CBEZ: {
				Value control1 = getValue(map, string("startControl"));
				Value control2 = getValue(map, string("endControl"));
				Value end = getValue(map, string("end"));

				Value* control1Array = AS_ARRAY(control1)->array->values;
				Value* control2Array = AS_ARRAY(control2)->array->values;
				Value* endArray = AS_ARRAY(end)->array->values;
				#ifdef EM_MAIN
					em_addCubicBezier(control1Array, control2Array, endArray);
				#else
					print(O_OUT, "cBezier ");
					printValue(O_OUT, control1);
					print(O_OUT, " ");
					printValue(O_OUT, control2);
					print(O_OUT, " ");
					printValue(O_OUT, end);
					print(O_OUT, "\n");
				#endif
			} break;
			case TK_ARC: {
				Value center = getValue(map, string("center"));
				Value* centerArray = AS_ARRAY(center)->array->values;

				Value degrees = getValue(map, string("degrees"));

				#ifdef EM_MAIN
					em_addArc(centerArray, &degrees);
				#else
					print(O_OUT, "Arc ");
					printValue(O_OUT, center);
					print(O_OUT, " ");
					printValue(O_OUT, degrees);
					print(O_OUT, "\n");
				#endif
			} break;
			case TK_MIRR: {
				Value origin = getValue(map, string("origin"));
				Value* originArray = AS_ARRAY(origin)->array->values;

				Value axis = getValue(map, string("axis"));
				#ifdef EM_MAIN
					em_addMirror(originArray, &axis);
				#else
					print(O_OUT, "Mirror ");
					printValue(O_OUT, origin);
					print(O_OUT, " ");
					printValue(O_OUT, axis);
					print(O_OUT, "\n");
				#endif
			} break;
			default:
				break;
		}
	}
	#ifdef EM_MAIN
		em_paintShape();
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

void setCanvas(){
	Value canvasMap = getValue(vm.globals, string("canvas"));
	if(IS_INST(canvasMap)){
		HashMap* innerMap = AS_INST(canvasMap)->map;
		Value originVal = getValue(innerMap, string("origin"));
		Value sizeVal = getValue(innerMap, string("size"));
		if(IS_ARRAY(originVal) && IS_ARRAY(sizeVal)){
			ObjArray* originVec = AS_ARRAY(originVal);
			ObjArray* sizeVec = AS_ARRAY(sizeVal);
			#ifdef EM_MAIN	
				em_setCanvas(originVec->array->values, sizeVec->array->values);
			#else
				print(O_OUT, "Canvas Origin: ");
				printValue(O_OUT, originVal);
				print(O_OUT, "\n");
				
				print(O_OUT, "Canvas Size: ");
				printValue(O_OUT, sizeVal);
				print(O_OUT, "\n");
			#endif
		}
	}
}

void renderFrame(CompilePackage* code){
	setCanvas();
	while(vm.shapeCount > 0){
		ObjShape* top = popShape();
		switch(top->shapeType){
			case TK_POLY:
			case TK_POLYL:
			case TK_PATH:
			case TK_UNGON:
				drawPoints(top);
				break;
			default:
				drawShape(top);	
				break;
		}
	}
}