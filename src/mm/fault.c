//  ***************************************************************
//  mm/fault.c - Creation date: 12/08/2022
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2022 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

// Namespace: Mf (Memory manager, page Fault)

#include <memory.h>
#include <idt.h>

int g_nPageFaultsSoFar = 0;//just for statistics


void MmOnPageFault(Registers *pRegs)
{
	g_nPageFaultsSoFar++;
	
	UserHeap *pHeap = MuGetCurrentHeap();
	
	LogMsg("Page fault happened at %x (error code: %x) on Heap %p", pRegs->cr2, pRegs->error_code, pHeap);
	
	if (!pHeap)
	{
		// Oh, no, we have no heap available, this must mean that this page fault is invalid!
		goto _INVALID_PAGE_FAULT;
	}
	
	union
	{
		struct
		{
			bool bPresent   : 1;
			bool bWrite     : 1;
			bool bUser      : 1;
			bool bResWrite  : 1;
			//...
		};
		
		uint32_t value;
	}
	errorCode;
	
	errorCode.value = pRegs->error_code;
	
	// if the page wasn't marked as PRESENT
	if (!errorCode.bPresent)
	{
		// Wasn't present...
		uint32_t* pPageEntry = MuGetPageEntryAt(pHeap, pRegs->cr2 & PAGE_BIT_ADDRESS_MASK, false);
		if (!pPageEntry)
		{
			goto _INVALID_PAGE_FAULT;
		}
		
		if (*pPageEntry & PAGE_BIT_PRESENT)
		{
			// Hrm? But the error code said it wasn't present, whatever
			return;
		}
		
		if (*pPageEntry & PAGE_BIT_DAI)
		{
			LogMsg("Page not present, allocating...");
			
			// It's time to map a page here
			uint32_t frame = MpFindFreeFrame();
			
			if (frame == 0xFFFFFFFF)
			{
				LogMsg("Out of memory, d'oh!");
				goto _INVALID_PAGE_FAULT;
			}
			
			MpSetFrame(frame << 12);
			
			*pPageEntry = *pPageEntry & 0xFFF;
			*pPageEntry |= frame << 12;
			*pPageEntry |= PAGE_BIT_PRESENT;
			*pPageEntry &= ~PAGE_BIT_DAI;
			
			MmInvalidateSinglePage(pRegs->cr2 & PAGE_BIT_ADDRESS_MASK);
			
			// Let's go, I guess?
			return;
		}
		
		// I can't understand?
		goto _INVALID_PAGE_FAULT;
	}
	
	if (!errorCode.bWrite)
	{
		// Should be a COW field. Was present...
		uint32_t* pPageEntry = MuGetPageEntryAt(pHeap, pRegs->cr2 & PAGE_BIT_ADDRESS_MASK, false);
		if (!pPageEntry)
		{
			// Da hell? But the error code said that the page was present
			goto _INVALID_PAGE_FAULT;
		}
		
		if (*pPageEntry & PAGE_BIT_COW)
		{
			LogMsg("Page supposed to be copied on write, allocating...");
			
			// It's show time...
			uint32_t frame = MpFindFreeFrame();
			
			if (frame == 0xFFFFFFFF)
			{
				LogMsg("Out of memory, d'oh!");
				goto _INVALID_PAGE_FAULT;
			}
			
			MpSetFrame(frame << 12);
			
			*pPageEntry = *pPageEntry & 0xFFF;
			*pPageEntry |= frame << 12;
			*pPageEntry |= PAGE_BIT_READWRITE;
			*pPageEntry &= ~PAGE_BIT_COW;
		}
	}
	
_INVALID_PAGE_FAULT:
	LogMsg("Invalid page fault");
	KeStopSystem();
}

