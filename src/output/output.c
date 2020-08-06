#include <stdarg.h>
#include <stdio.h>
#include "common.h"
#include "output.h"

#ifdef EM_MAIN
	extern void em_printOut(const char* c);
	extern void em_printDebug(const char* c);
	extern void em_printError(const char* c);
#endif

void vprint(OutType type, const char* c, va_list arglist){
	#ifdef EM_MAIN
		char line[1024];
		vsprintf(line, c, arglist);
		switch(type){
			case O_DEBUG:
				em_printDebug(line);
				break;
			case O_ERR:
				em_printError(line);
				break;
			case O_OUT:
				em_printOut(line);
				break;
		}
	#else
		vprintf(c, arglist);
	#endif
}

void print(OutType type, const char* c, ...){
	va_list arglist;
	va_start(arglist, c);

	#ifdef EM_MAIN
		char line[1024];
		vsprintf(line, c, arglist);

		switch(type){
			case O_DEBUG:
				em_printDebug(line);
				break;
			case O_ERR:
				em_printError(line);
				break;
			case O_OUT:
				em_printOut(line);
				break;
		}
	#else
		vprintf(c, arglist);

	#endif
		va_end(arglist);
}

void printMem(const char* c) {
	float inKB = numBytesAllocated / (float) 1024;
	printf("[A] - %s: %.2f kb\n", c, inKB);
}