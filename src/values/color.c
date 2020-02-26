#include <stdio.h>
#include "color.h"
#include "memory.h"
#include "value.h"
#include "output.h"
#include "svg.h"
#include "obj.h"


ObjArray* allocateColor(double r, double g, double b, double a){
	ObjArray* arrayObj = allocateArray();
	addValues(arrayObj->array, 4, r, g, b, a);
	return arrayObj;
}

#ifdef EM_MAIN
/*void addColorAttribute(const char* key, Color* cl){
	if(cl){
		char* colorString = allocateColorString(cl);
		addStringStyle(key, );
		FREE(char, colorString);
	}else{
		addStringStyle(key, "rgba(0, 0, 0, 1)");
	}
}*/
#endif