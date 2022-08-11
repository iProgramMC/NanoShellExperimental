#include <main.h>
#include <memory.h>
#include <print.h>
#include <idt.h>
#include <keyboard.h>
#include <elf.h>
#include <multiboot.h>

void KeStopSystem()
{
	cli;
	while (1)
		hlt;
}

extern uint32_t e_placement;

void FreeTypeThing()
{
	LogMsgNoCr("\nType something! >");
	
	char test[2];
	test[1] = '\0';
	while (1)
	{
		char k = KbWaitForKeyAndGet();
		test[0] = k;
		LogMsgNoCr(test);
		hlt;
	}
}

void MhTest()
{
	uint32_t *page1 = MhAllocateSinglePage(NULL);
	uint32_t *page2 = MhAllocateSinglePage(NULL);
	uint32_t *page3 = MhAllocateSinglePage(NULL);
	
	page1[5] = 0x13371337;
	page2[7] = 0xbeefbeef;
	
	LogMsg("Pages 1, 2, 3: %p %p %p. Page1[5]=%x. Page2[7]=%x", page1, page2, page3, page1[5], page2[7]);
	
	MhFree(page2); page2 = NULL;
	
	uint32_t *page4 = MhAllocateSinglePage(NULL);
	
	LogMsg("Page 4: %p. Page 2: %p", page4, page2);
	
	page4[1023] = 0xFFCC9966;
	LogMsg("Page4[1023]=%x.", page4[1023]);
	
	MhFree(page1);
	MhFree(page2);
	MhFree(page3);
	MhFree(page4);
	
}

extern int g_numPagesAvailable;
void MmInitializePMM(multiboot_info_t* pInfo);

void KeStartupSystem (unsigned long magic, unsigned long mbaddr)
{
	PrInitialize();
	if (magic != 0x2badb002)
	{
		LogMsg("Sorry, this ain't a compatible multiboot bootloader.");
		KeStopSystem();
	}
	mbaddr += 0xc0000000; //turn it virtual straight away
	
	multiboot_info_t *mbi = (multiboot_info_t*)mbaddr;
	
	int nKbExtRam = mbi->mem_upper; //TODO: use multiboot_info_t struct
	
	if (nKbExtRam < 8192)
	{
		LogMsg("NanoShell has not found enough extended memory.  8Mb of extended memory is\nrequired to run NanoShell.  You may need to upgrade your computer.");
		KeStopSystem();
	}
	
	MpInitialize(mbi);
	MhInitialize();
	
	KeIdtInit();
	
	//print the hello text, to see if the os booted properly
	LogMsg("NanoShell Operating System " VersionString);
	LogMsg("[%d Kb System Memory, %d Kb Usable Memory]", nKbExtRam, g_numPagesAvailable * 4	);
	
	LogMsg("There are %d pages available to the system.", g_numPagesAvailable);
	
	MhTest();
	
	FreeTypeThing();
	
	KeStopSystem();
}