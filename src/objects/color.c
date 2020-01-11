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
			char* result = ALLOCATE(char, 18);
			int r = (int) AS_NUM(rgb->r);
			int g = (int) AS_NUM(rgb->g);
			int b = (int) AS_NUM(rgb->b);
			sprintf(result, "rgb(%d, %d, %d)", r, g, b);
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
	char* colorString = allocateColorString(cl);
	addStringStyle(key, colorString, -1, -1);
	FREE(char, colorString);
}
#endif