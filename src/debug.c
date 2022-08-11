#include <debug.h>

void DumpRegisters (Registers* pRegs)
{
	LogMsg("Registers:");
	LogMsg("EAX=%x CS=%x "    "EIP=%x EFLGS=%x", pRegs->eax, pRegs->cs, pRegs->eip, pRegs->eflags);
	LogMsg("EBX=%x             ESP=%x EBP=%x",   pRegs->ebx,            pRegs->esp, pRegs->ebp);
	LogMsg("EBX=%x             ESI=%x", pRegs->ecx,            pRegs->esi);
	LogMsg("EBX=%x             EDI=%x", pRegs->edx,            pRegs->edi);
}
