#include <debug.h>

//debug:
/*
const char *g_pInterruptDisablerFile = NULL;
int         g_nInterruptDisablerLine = 0;
const char *g_pInterruptEnablerFile = NULL;
int         g_nInterruptEnablerLine = 0;
*/

void DumpRegisters (Registers* pRegs)
{
	LogMsg("Registers:");
	LogMsg("EAX=%x CS=%x "    "EIP=%x EFLGS=%x", pRegs->eax, pRegs->cs, pRegs->eip, pRegs->eflags);
	LogMsg("EBX=%x             ESP=%x EBP=%x",   pRegs->ebx,            pRegs->esp, pRegs->ebp);
	LogMsg("EBX=%x             ESI=%x", pRegs->ecx,            pRegs->esi);
	LogMsg("EBX=%x             EDI=%x", pRegs->edx,            pRegs->edi);
}

bool OnAssertionFail (const char *pStr, const char *pFile, const char *pFunc, int nLine)
{
	LogMsg("ASSERTION FAILED!");
	LogMsg("%s", pStr);
	LogMsg("At %s:%d [%s]", pFile, nLine, pFunc);
	
	LogMsg("System will now stop.");
	KeStopSystem();
	
	return false;
}

void KeStopSystem()
{
	//no point in calling KeDisableInterruptsD when that can result in a crash
	__asm__ volatile ("cli");
	
	while (1)
		hlt;
}

bool KeCheckInterruptsDisabled()
{
	// TODO FIXME: Don't use this hacky method, keep track of
	// it ourselves. Unfortunately that wouldn't work with the
	// page fault handler since, you know, it can call itself
	// (due to the phys mem reference counter being utter ass)
	
	return !(KeGetEFlags() & EFLAGS_IF);
	//return !g_bAreInterruptsEnabled;
}

void KeVerifyInterruptsDisabledD(const char * file, int line)
{
	if (!KeCheckInterruptsDisabled())
	{
		SLogMsg("Interrupts are NOT disabled! (called from %s:%d)", file, line);
		ASSERT(!"Hmm, interrupts shouldn't be enabled here");
	}
}

void KeDisableInterruptsD(const char * file, int line)
{
	//on debug, also check if we've already disabled the interrupts
	/*
	if (!g_bAreInterruptsEnabled)
	{
		SLogMsg("Interrupts are already disabled! (called from %s:%d, but they have already been disabled from %s:%d)", file, line, g_pInterruptDisablerFile, g_nInterruptDisablerLine);
		KeStopSystem();
	}
	*/
	__asm__ volatile ("cli");
	/*
	g_bAreInterruptsEnabled = false;
	g_pInterruptDisablerFile = file;
	g_nInterruptDisablerLine = line;
	*/
}

void KeEnableInterruptsD(const char * file, int line)
{
	//on debug, also check if we've already enabled the interrupts
	/*
	if (g_bAreInterruptsEnabled)
	{
		SLogMsg("Interrupts are already enabled! (called from %s:%d, but they have already been enabled from %s:%d)", file, line, g_pInterruptEnablerFile, g_nInterruptEnablerLine);
		KeStopSystem();
	}
	
	g_pInterruptEnablerFile = file;
	g_nInterruptEnablerLine = line;
	g_bAreInterruptsEnabled = true;
	*/
	__asm__ volatile ("sti");
}
