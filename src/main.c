#include <main.h>
#include <memory.h>
#include <print.h>
#include <idt.h>
#include <keyboard.h>
#include <elf.h>
#include <multiboot.h>

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
	uint32_t *page3 = MhAllocateSinglePage(NULL);
	uint32_t *page2 = MhAllocate(3 * PAGE_SIZE, NULL);
	
	page1[5] = 0x13371337;
	page2[2 * PAGE_SIZE	/4 + 7] = 0xbeefbeef;
	
	LogMsg("Pages 1, 2, 3: %p %p %p. Page1[5]=%x. Page2[7]=%x. Page2[2*PAGE_SIZE+7]=%x", page1, page2, page3, page1[5], page2[7], page2[2 * PAGE_SIZE/4	 + 7]);
	
	page2 = MhReAllocate(page2, 6*PAGE_SIZE);
	page2[6000] = 0x12345678;
	
	LogMsg("Reallocated, Page2: %p. Page2[2*PAGE_SIZE/4+7]=%x. Page2[6000]=%x.",page2,page2[2*PAGE_SIZE/4+7],page2[6000]);
	
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

void MuTestWriteAll(uint8_t* pMem, size_t size)
{
	// Test the writing.
	for (size_t i = 0; i < size; i++)
	{
		pMem[i] = 0x55 ^ i;
	}
	
	// Test the reading.
	for (size_t i = 0; i < size; i++)
	{
		uint8_t thing = (0x55 ^ i);
		if (pMem[i] != thing)
		{
			LogMsg("Write failed (%p size %d) at %d", pMem, size, i);
		}
	}
}

void MuTest()
{
	LogMsg("Creating User heap");
	
	UserHeap *pHeap = MuCreateHeap();
	
	// use the heap
	MuUseHeap(pHeap);
	
	LogMsg("Hello from heap context");
	
	MuCreateMapping (pHeap, 0x12345000, 0x00400000, true); //at 4M
	
	*((uint32_t*)0x12345678) = 0x13371337;
	LogMsg("Wrote");
	
	LogMsg("Logging heap list:");
	MuDebugLogHeapList(pHeap);
	LogMsg("Logging heap list Done");
	
	//MuRemoveMapping (pHeap, 0x12345000, true);
	
	//we can still read it from 0xC0400000
	LogMsg("What we wrote: %x", *((uint32_t*)0xC0400678));
	
	LogMsg("Trying out a 32k mapping at 0x00001000");
	
	void *pMemory = (void*)0x00001000;
	
	//MuMapMemoryFixedHint(pHeap, (uintptr_t)pMemory, 8, NULL, true, false, false);
	
	//map some things for an attempt
	MuMapMemory(pHeap, 423, NULL, &pMemory, true, false);
	MuMapMemory(pHeap, 64, NULL, &pMemory, true, false);
	MuMapMemory(pHeap, 876, NULL, &pMemory, true, false);
	
	if (!MuMapMemory(pHeap, 8, NULL, &pMemory, true, false))
	{
		LogMsg("Failed! Weird.");
	}
	else
	{
		LogMsg("Got pMemory=%p", pMemory);
		
		MuTestWriteAll((uint8_t*)pMemory, 8*PAGE_SIZE);
		LogMsg("Done. Any errors? Don't think so.");
	}
	
	LogMsg("Logging heap list:");
	MuDebugLogHeapList(pHeap);
	LogMsg("Logging heap list Done");
	
	// Clone the heap.
	UserHeap *pNewHeap = MuCloneHeap(pHeap);
	
	MuUseHeap(pNewHeap);
	
	// Write to pMemory
	*((uint32_t*)pMemory) = 0xCAFEBABE;
	LogMsg("Wrote to pMemory=%p on heap %p: %x %x", pMemory, pNewHeap, *((uint32_t*)pMemory), *((uint32_t*)pMemory + 1));
	
	MuUseHeap(pHeap);
	
	LogMsg("On old heap it is %x %x", *((uint32_t*)pMemory), *((uint32_t*)pMemory + 1));
	
	MuResetHeap();
	
	MuKillHeap(pHeap);
	MuKillHeap(pNewHeap);
	pHeap = NULL;
	pNewHeap = NULL;
	
	LogMsg("Hello I'm back!");
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
	LogMsg("NanoShell Experimental Operating System " VersionString);
	LogMsg("[%d Kb System Memory, %d Kb Usable Memory]", nKbExtRam, g_numPagesAvailable * 4	);
	
	LogMsg("There are %d pages available to the system, %d of which are still free.", g_numPagesAvailable, MpGetNumFreePages());
	
	MhTest();
	
	MuTest();
	
	LogMsg("There are now %d out of %d pages available to the system.", MpGetNumFreePages(), g_numPagesAvailable);
	LogMsg("We have faulted %d times.", MmGetNumPageFaults());
	
	FreeTypeThing();
	
	KeStopSystem();
}