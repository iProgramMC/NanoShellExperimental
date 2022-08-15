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
		// TODO: What if this overflows the stack? hmm..
		//g_root.m_level1[aSplit.level1] = M
	}
	
}
