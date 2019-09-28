#ifndef scute_output_h
#define scute_output_h
#include <stdarg.h>

typedef enum {
	O_DEBUG,
	O_ERR,
	O_OUT
} OutType;

void print(OutType type, const char* c, ...);
void vprint(OutType type, const char* c, va_list args);

void vprintln(OutType type, const char* c, ...);
void println(OutType type, const char* c, ...);

#endif