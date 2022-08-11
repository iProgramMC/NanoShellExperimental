#ifndef _PRINT_H
#define _PRINT_H

#include <main.h>
#include <stdarg.h>

size_t sprintf(char*a, const char*c, ...);
size_t vsprintf(char* memory, const char* format, va_list list);
void LogInt (uint32_t i);
void LogIntDec (int i);
void KePrintSystemInfo();
void PrInitialize();

#endif//_PRINT_H