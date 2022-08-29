#ifndef _DEBUG_H
#define _DEBUG_H

#include <syscall.h>

#define EFLAGS_IF  (1 << 9)

void DumpRegisters (Registers*);
void IntHandlerStart();
void IntHandlerEnd();

#endif//_DEBUG_H