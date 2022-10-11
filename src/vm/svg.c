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


bool fallsWithinTimeline(int lower, int upper, int current){
	return (current >= lower) && (current <= upper);
}

void renderAnimationBlocks(CompilePackage* package, int timeIndex){
	for(int i = 0; i<package->numAnimations; ++i){
		ObjAnim* anim = package->animations[i];
		HashEntry* currentEntry = anim->map->first;
		#ifdef EM_MAIN
			em_initAnimationChunk(anim->shape);
		#endif
		while(currentEntry != NULL){
			char* property = currentEntry->key->chars;
			ObjTimeline* timeline = (ObjTimeline*) AS_OBJ(currentEntry->value);
			Timestep* previous = NULL;
			for(int i = timeline->stepIndex; i<timeline->numSteps; ++i){
				Timestep* step = &timeline->steps[i];

				if(fallsWithinTimeline(step->min, step->max, timeIndex)){
					Value* previousValue = previous ? &previous->resolved : NULL;
					Value executedValue = executeThunk(step->thunk, timeIndex, previousValue);
					step->resolved = executedValue;
					switch(step->resolved.type){
						case VL_OBJ:{
							Obj* valueAsObject = AS_OBJ(step->resolved);
							switch(valueAsObject->type){
								default: {
									runtimeError("Animation value type not currently supported.");
								} break;
							}
						}
						default: {
							#ifdef EM_MAIN
								em_animateValue(currentEntry->key->chars, &step->resolved);
							#else
								printValue(O_OUT, step->resolved);
								print(O_OUT, "\n");
							#endif
						} break;
					}				
				}else{
					++timeline->stepIndex;
				}
				previous = step;
			}
			currentEntry = currentEntry->next;
		}
		#ifdef EM_MAIN
			em_finalizeAnimationChunk();
		#endif
	}
}

void drawShape(ObjShape* shape, unsigned address){
	HashMap* shapeMap = shape->instance.map;
	TKType type = shape->shapeType;
	#ifdef EM_MAIN
		em_assignAnimation(shape->animation);
		switch(type){
			case TK_RECT: {
				em_newRect(address);

				Value posVal = getValue(shapeMap, string("position"));
				Value sizeVal = getValue(shapeMap, string("size"));
				Value roundingVal = getValue(shapeMap, string("rounding"));

				ValueArray* sizeArray = AS_ARRAY(sizeVal)->array;
				ValueArray* posArray = AS_ARRAY(posVal)->array;
				
				em_addVectorAttr("size", sizeArray->values);
				em_addVectorAttr("position", posArray->values);
				em_addAttribute("rounding", &roundingVal);
			} break;

			case TK_CIRCLE:{
				em_newCirc(address);

				Value posVal = getValue(shapeMap, string("position"));
				Value radVal = getValue(shapeMap, string("radius"));
				ValueArray* posArray = AS_ARRAY(posVal)->array;
				
				em_addVectorAttr("position", posArray->values);
				em_addAttribute("radius", &radVal);
				} break;

			case TK_ELLIPSE: {
				em_newEllip(address);

				Value posVal = getValue(shapeMap, string("position"));
				Value radVal = getValue(shapeMap, string("radius"));

				ValueArray* posArray = AS_ARRAY(posVal)->array;
				ValueArray* radArray = AS_ARRAY(radVal)->array;

				em_addVectorAttr("position", posArray->values);
				em_addVectorAttr("radius", posArray->values);
				} break;

			case TK_LINE: {
				em_newLine(address);

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
		assignStyles(shape);
		em_paintShape(shape);
		#else	
			printMap(O_OUT, shapeMap, 0);
		#endif
}

double toRadians(int degrees){
	return (3.14/180) * degrees;
}

void drawPoints(ObjShape* shape){
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
					em_addJump(segment, vector->array->values);
				#else
					print(O_OUT, "Jump: (%d, %d)\n", points[0], points[1]);
				#endif
			} break;
			case TK_VERTEX: ;
				ObjArray* vector = AS_ARRAY(getValue(map, string("position")));
				points[0] = AS_NUM(getValueArray(vector->array, 0));
				points[1] = AS_NUM(getValueArray(vector->array, 1));
				#ifdef EM_MAIN
					em_addVertex(segment, vector->array->values);
				#else
					print(O_OUT, "Vertex: (%d, %d)\n", points[0], points[1]);
				#endif
				break;
			case TK_MOVE: ;
				Value distance = getValue(map, string("distance"));
				points[0] += (int) round(cos(toRadians(angle))*AS_NUM(distance));
				points[1] += (int) round(sin(toRadians(angle))*AS_NUM(distance));
				#ifdef EM_MAIN
					em_addMove(segment, &distance, &angle);				
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
			case TK_QBEZIER: {
				Value control = getValue(map, string("control"));
				Value end = getValue(map, string("end"));

				Value* controlArray = AS_ARRAY(control)->array->values;
				Value* endArray = AS_ARRAY(end)->array->values;
				#ifdef EM_MAIN
					em_addQuadBezier(segment, controlArray, endArray);
				#else
					print(O_OUT, "qBezier ");
					printValue(O_OUT, control);
					print(O_OUT, " ");
					printValue(O_OUT, end);
					print(O_OUT, "\n");
				#endif
			} break;
			case TK_CBEZIER: {
				Value control1 = getValue(map, string("startControl"));
				Value control2 = getValue(map, string("endControl"));
				Value end = getValue(map, string("end"));

				Value* control1Array = AS_ARRAY(control1)->array->values;
				Value* control2Array = AS_ARRAY(control2)->array->values;
				Value* endArray = AS_ARRAY(end)->array->values;
				#ifdef EM_MAIN
					em_addCubicBezier(segment, control1Array, control2Array, endArray);
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
					em_addArc(segment, centerArray, &degrees);
				#else
					print(O_OUT, "Arc ");
					printValue(O_OUT, center);
					print(O_OUT, " ");
					printValue(O_OUT, degrees);
					print(O_OUT, "\n");
				#endif
			} break;
			case TK_MIRROR: {
				Value origin = getValue(map, string("origin"));
				ObjArray* originObj = (ObjArray*) AS_OBJ(origin);
				Value* originArray = originObj->array->values;

				Value axis = getValue(map, string("axis"));
				CSType axisType = (CSType) AS_NUM(axis);
				bool isX = axisType == CS_X || axisType == CS_XY;
				bool isY = axisType == CS_Y || axisType == CS_XY;
				#ifdef EM_MAIN
					em_addMirror(
						segment,
						originArray,
						isX, 
						isY
					);
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
		em_paintShape(shape);
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
		ObjShape* shape = popShape();
		unsigned address = (uintptr_t) shape->instance.map;
		switch(shape->shapeType){
			case TK_POLYGON: {
				#ifdef EM_MAIN
					em_newPolygon(address);
					assignStyles(shape);
					em_assignAnimation(shape->animation);
				#endif
				drawPoints(shape);
			} break;
			case TK_POLYLINE: {
				#ifdef EM_MAIN
					em_newPolyline(address);
					assignStyles(shape);
					em_assignAnimation(shape->animation);
				#endif
				drawPoints(shape);
			} break;
			case TK_PATH: {
				#ifdef EM_MAIN
					em_newPath(address);
					assignStyles(shape);
					em_assignAnimation(shape->animation);
					Value closed = getValue(shape->instance.map, string("closed"));
					em_addAttribute("closed", &closed);
				#endif
				drawPoints(shape);
			} break;
			case TK_UNGON: {
				#ifdef EM_MAIN
					em_newUngon(address);
					assignStyles(shape);
					em_assignAnimation(shape->animation);
					Value closed = getValue(shape->instance.map, string("closed"));
					em_addAttribute("closed", &closed);
				#endif
				drawPoints(shape);
			} break;
			default: {
				drawShape(shape, address);
			} break;
		}
	}
}