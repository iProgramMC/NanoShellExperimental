//  ***************************************************************
//  mm/pmrcount.c - Creation date: 14/08/2022
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2022 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

// This module deals with storing reference counts to physical pages.

#include <memory.h> // I'm not sure we need all that crap anyway, just include it for the structure definition we'll be using
#include <string.h>

RefCountTableLevel0 g_root;

uint32_t MrGetReferenceCount(uintptr_t page)
{
	// Split the address up into chunks
	union
	{
		struct
		{
			uint32_t pageOffset: 12; //won't use this, it's there just to offset by 12 bytes
			uint32_t level2 : 10;
			uint32_t level1 : 10;
		};
		uintptr_t address;
	}
	aSplit;
	
	aSplit.address = page;
	
	// Is there a level1?
	if (!g_root.m_level1[aSplit.level1])
	{
		// No, assume this page was never referenced
		return 0;
	}
	
	// There is a level1.
	return g_root.m_level1[aSplit.level1]->m_refCounts[aSplit.level2];
}

// Returns the new reference count
uint32_t MrReferencePage(uintptr_t page)
{
	LogMsg("MrReferencePage(%p)",page);
	// Split the address up into chunks
	union
	{
		struct
		{
			uint32_t pageOffset: 12; //won't use this, it's there just to offset by 12 bytes
			uint32_t level2 : 10;
			uint32_t level1 : 10;
		};
		uintptr_t address;
	}
	aSplit;
	
	aSplit.address = page;
	
	// Is there a level1?
	if (!g_root.m_level1[aSplit.level1])
	{
		// No, make one
		// TODO: What if this overflows the stack? hmm.., shouldn't happen
		// unless you manipulate the PMM to only have one page available per
		// 4m cluster
		g_root.m_level1[aSplit.level1] = MhAllocateSinglePage(NULL);
		
		LogMsg("g_root.m_level1[%d] = %p", aSplit.level1, g_root.m_level1[aSplit.level1]);
		
		if (!g_root.m_level1[aSplit.level1])
		{
			LogMsg("Uh oh, can't reference count physical page %x", page);
			KeStopSystem();
			return 0;
		}
		
		memset(g_root.m_level1[aSplit.level1], 0, PAGE_SIZE);
	}
	
	uint32_t* pRefCount = &g_root.m_level1[aSplit.level1]->m_refCounts[aSplit.level2];
	
	return ++(*pRefCount);
}

uint32_t MrUnreferencePage(uintptr_t page)
{
	// Split the address up into chunks
	union
	{
		struct
		{
			uint32_t pageOffset: 12; //won't use this, it's there just to offset by 12 bytes
			uint32_t level2 : 10;
			uint32_t level1 : 10;
		};
		uintptr_t address;
	}
	aSplit;
	
	aSplit.address = page;
	
	// Is there a level1?
	if (!g_root.m_level1[aSplit.level1])
	{
		// No
		LogMsg("Couldn't unreference physical page %x, was never referenced?", page);
		KeStopSystem();
	}
	
	uint32_t* pRefCount = &g_root.m_level1[aSplit.level1]->m_refCounts[aSplit.level2];
	
	uint32_t result = --(*pRefCount);
	
	if (!*pRefCount)
	{
		// The reference count is zero, it's time to see if all other pages are set to zero so that we free this level1
		bool bNotAllZeroes = false;
		for (int i = 0; i < 1024; i++)
		{
			if (g_root.m_level1[aSplit.level1]->m_refCounts[i] != 0)
			{
				bNotAllZeroes = true;
				break;
			}
		}
		
		if (!bNotAllZeroes)
		{
			// we can free this!!
			LogMsg("Freeing empty level1 at %x (%p)", aSplit.level1, page);
			MhFree(g_root.m_level1[aSplit.level1]);
			g_root.m_level1[aSplit.level1] = NULL;
		}
	}
	
	return result;
}
