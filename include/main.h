#ifndef _MAIN_H
#define _MAIN_H

#include<stddef.h>
#include<stdarg.h>
#include<stdint.h>

typedef char bool;
typedef char byte;
typedef char BYTE;
typedef unsigned uint;

#define false 0
#define true 1

#define hlt __asm__("hlt\n\t")  
//#define cli __asm__("cli\n\t")
//#define sti __asm__("sti\n\t")
#define cli KeDisableInterruptsD(__FILE__, __LINE__)
#define sti KeEnableInterruptsD (__FILE__, __LINE__)

void KeDisableInterruptsD(const char * file, int line);
void KeEnableInterruptsD (const char * file, int line);

#define Version 10
#define VersionString "V0.10"

#define crash __asm__("int $0x10\n\t") // Int 0x10 doesn't work in pmode! Might as well make use of it.

#define KERNEL_MEM_START 0xC0000000

extern void WritePort(unsigned short port, unsigned char data);
extern unsigned char ReadPort(unsigned short port);
extern void WritePortW(unsigned short port, unsigned short data);
extern unsigned short ReadPortW(unsigned short port);

//note: needs to be used for arrays only (So no pointers, otherwise it'll go weird)
#define ARRAY_COUNT(array) (sizeof(array)/sizeof(*array))

#define STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)

#define UNUSED __attribute__((unused))

// The function that's called when an assertion fails.
bool OnAssertionFail (const char *pStr, const char *pFile, const char *pFunc, int nLine);
#define ASSERT(condition) ((condition) || OnAssertionFail(#condition, __FILE__, __FUNCTION__, __LINE__))

// Stops the system immediately.
void KeStopSystem(void);

// Gets the contents of the EFLAGS register.
uint32_t KeGetEFlags(void);

// Checks if interrupts are disabled right now.
bool KeCheckInterruptsDisabled(void);

// Asserts if interrupts are disabled.
void KeVerifyInterruptsDisabledD(const char * file, int line);
#define KeVerifyInterruptsDisabled KeVerifyInterruptsDisabledD(__FILE__, __LINE__)

#include <console.h>

#endif//_MAIN_H