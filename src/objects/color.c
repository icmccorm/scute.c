#include <stdio.h>
#include "color.h"
#include "memory.h"
#include "value.h"
#include "output.h"
#include "svg.h"

char* allocateColorString(Color* cl){
	switch(cl->colorType){
		case CL_RGB: ;
			ColorRGB* rgb = (ColorRGB*) cl;
			char* result = ALLOCATE(char, 32);
			int r = (int) AS_NUM(rgb->r);
			int g = (int) AS_NUM(rgb->g);
			int b = (int) AS_NUM(rgb->b);
			float a = (float) AS_NUM(rgb->a);
			sprintf(result, "rgba(%d, %d, %d, %f)", r, g, b, a);
			return result;
			break;
		default:
			break;
	}
	return NULL;
}

void printColor(OutType out, Color* cl){
	char * colorString = allocateColorString(cl);
	print(out, colorString);
	FREE(char, colorString);
}

#ifdef EM_MAIN
void addColorAttribute(const char* key, Color* cl){
	if(cl){
		char* colorString = allocateColorString(cl);
		addStringStyle(key, colorString, -1);
		FREE(char, colorString);
	}else{
		addStringStyle(key, "rgba(0, 0, 0, 1)", -1);
	}
}
#endif