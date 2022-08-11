//
// Memory and string processing
// Copyright (C) 2020 iProgramInCpp.
//
#ifndef _STRING_H
#define _STRING_H
#include <main.h>

// Memory functions
int memcmp(const void*ap,const void*bp,size_t size);
void* memcpy(void* dstptr, const void* srcptr, size_t size);// TODO: I don't think  we need restrict keyword here
void* memmove(void* restrict dstptr, const void* restrict srcptr, size_t size);
void* memset(void* bufptr, BYTE val, size_t size);
void memtolower(char* as, int w);
void memtoupper(char* as, int w);
void* fast_memset(void* bufptr, BYTE val, size_t size);

// String functions
size_t strgetlento(const char* str, char chr);
int atoi(const char* str) ;
size_t strlen(const char* str) ;
void* strcpy(const char* ds, const char* ss);
void strtolower(char* as);
void strtoupper(char* as);
int strcmp(const char* as, const char* bs);
void strcat(char* dest, char* after);
void fmemcpy32 (void* restrict dest, const void* restrict src, size_t size);

//requires 4 byte aligned size.
void ZeroMemory (void* bufptr1, size_t size);

#endif//_STRING_H