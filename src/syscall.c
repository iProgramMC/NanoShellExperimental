#include <syscall.h>
#include <debug.h>
#include <userspace/syscalls.h>

void OnSyscallReceived (Registers* pRegs)
{
	switch (pRegs->esi)
	{
		case LOGMSG:
			LogMsg("%s", (char*)pRegs->eax);//avoid parsing %s's as something off the stack!
			break;
		default:
			LogMsg("warning: undefined syscall");
			DumpRegisters(pRegs);
			break;
	}
}