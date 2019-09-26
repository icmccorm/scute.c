#include <stdarg.h>
#include <stdio.h>
#include "output.h"

#ifdef EM_MAIN
	extern void printOut(const char* c);
	extern void printDebug(const char* c);
	extern void printError(const char* c);
#endif

void vprint(OutType type, const char* c, va_list arglist){
	#ifdef EM_MAIN
		char line[1024];
		vsprintf(line, c, arglist);
		switch(type){
			case O_DEBUG:
				printDebug(line);
				break;
			case O_ERR:
				printError(line);
				break;
			case O_OUT:
				printOut(line);
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
				printDebug(line);
				break;
			case O_ERR:
				printError(line);
				break;
			case O_OUT:
				printOut(line);
				break;
		}
	#else
		vprintf(c, arglist);

	#endif
		va_end(arglist);
}