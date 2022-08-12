#ifndef _SYSCALL_H
#define _SYSCALL_H

#include <main.h>
#include <idt.h>

void OnSyscallReceived (Registers* pRegs);


#endif//_SYSCALL_H