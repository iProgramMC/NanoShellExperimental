//  ***************************************************************
//  mm/uheap.c - Creation date: 11/08/2022
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2022 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

// Namespace: Mu (Memory manager, User heap)

#include <memory.h>
#include <string.h>

UserHeap* g_pCurrentUserHeap;

extern uint32_t g_KernelPageDirectory[];

UserHeap* MuCreateHeap()
{
	UserHeap *pHeap = MhAllocate(sizeof(UserHeap), NULL);
	
	pHeap->m_nPageDirectory = 0;
	pHeap->m_nMappingHint   = USER_HEAP_BASE;
	pHeap->m_pPageDirectory = MhAllocateSinglePage(&pHeap->m_nPageDirectory);
	memset(pHeap->m_pPageDirectory, 0, PAGE_SIZE);
	
	// Copy the latter half from the kernel heap.
	for (int i = 0x200; i < 0x400; i++)
		pHeap->m_pPageDirectory[i] = g_KernelPageDirectory[i];
	
	memset(pHeap->m_pPageTables, 0, sizeof(pHeap->m_pPageTables));
	
	return pHeap;
}

UserHeap* MuCloneHeap(UserHeap* pHeapToClone)
{
	//TODO. In order to do this efficiently, I think we should add COW first
	return NULL;
}

void MuKillPageTablesEntries(uint32_t* pPageTable)
{
	// Free each of the pages, if there are any
	for (int i = 0; i < 0x400; i++)
	{
		if (pPageTable[i] & PAGE_BIT_PRESENT)
		{
			uint32_t frame = pPageTable[i] & PAGE_BIT_ADDRESS_MASK;
			
			MpClearFrame(frame);
			
			pPageTable[i] = 0;
			
			//TODO: Determine what page table this is so we can invalidate the page
		}
	}
}

void MuKillHeap(UserHeap* pHeap)
{
	// To kill the heap, first we need to empty all the page tables
	for (int i = 0; i < 0x200; i++)
	{
		if (pHeap->m_pPageTables[i])
		{
			MuRemovePageTable(pHeap, i);
		}
	}
	
	// Should be good to go.  Free the page directory, and then the heap itself
	MhFree(pHeap->m_pPageDirectory);
	MhFree(pHeap);
}

void MuCreatePageTable(UserHeap *pHeap, int pageTable)
{
	// Reset the page table there.
	pHeap->m_pPageDirectory[pageTable] = 0;
	
	// Create a new page table page on the kernel heap.
	pHeap->m_pPageTables[pageTable] = MhAllocateSinglePage(&pHeap->m_pPageDirectory[pageTable]);
	memset(pHeap->m_pPageTables[pageTable], 0, PAGE_SIZE);
	
	// Assign its bits, too.
	pHeap->m_pPageDirectory[pageTable] |= PAGE_BIT_PRESENT | PAGE_BIT_READWRITE;
}

void MuRemovePageTable(UserHeap *pHeap, int pageTable)
{
	// If we have a page table there...
	if (pHeap->m_pPageTables[pageTable])
	{
		// TODO: ensure that there are no page entries
		MuKillPageTablesEntries(pHeap->m_pPageTables[pageTable]);
		
		// reset it to zero
		pHeap->m_pPageDirectory[pageTable] = 0;
		
		// Get rid of the page we've allocated for it.
		MhFree(pHeap->m_pPageTables[pageTable]);
		pHeap->m_pPageTables[pageTable] = NULL;
	}
}

uint32_t* MuGetPageEntryAt(UserHeap* pHeap, uintptr_t address, bool bGeneratePageTable)
{
	// Split the address up into chunks
	union
	{
		struct
		{
			uint32_t pageOffset: 12; //won't use this, it's there just to offset by 12 bytes
			uint32_t pageEntry : 10;
			uint32_t pageTable : 10;
		};
		uintptr_t address;
	}
	addressSplit;
	
	addressSplit.address = address;
	
	// Is there a page table there?
	if (!pHeap->m_pPageTables[addressSplit.pageTable])
	{
		// No. Create one if needed.
		if (bGeneratePageTable)
			MuCreatePageTable(pHeap, addressSplit.pageTable);
		else
			return NULL;
	}
	
	if (!pHeap->m_pPageTables[addressSplit.pageTable])
	{
		// Still not? Guess something failed then.
		return NULL;
	}
	
	// Alright, grab a reference to the page entry, and return it.
	uint32_t* pPageEntry = &pHeap->m_pPageTables[addressSplit.pageTable][addressSplit.pageEntry];
	return pPageEntry;
}

// Create a mapping at `address` to point to `physAddress` in the user heap `pHeap`.
bool MuCreateMapping(UserHeap *pHeap, uintptr_t address, uint32_t physAddress, bool bReadWrite)
{
	uint32_t* pPageEntry = MuGetPageEntryAt(pHeap, address, true);
	
	if (!pPageEntry)
	{
		// If the page entry couldn't be created, return false because we could not map. :^(
		return false;
	}
	
	// Is there a page entry already?
	if (*pPageEntry & PAGE_BIT_PRESENT)
	{
		// Yeah... For obvious reasons we can't map here
		return false;
	}
	
	*pPageEntry = physAddress & PAGE_BIT_ADDRESS_MASK;
	*pPageEntry |= PAGE_BIT_PRESENT;
	if (bReadWrite)
		*pPageEntry |= PAGE_BIT_READWRITE;
	
	// WORK: This might not actually be needed if TLB doesn't actually cache non-present pages. Better to be on the safe side, though
	MmInvalidateSinglePage(address);
	
	return true;
}

bool MuRemoveMapping(UserHeap *pHeap, uintptr_t address)
{
	uint32_t* pPageEntry = MuGetPageEntryAt(pHeap, address, false);
	
	if (!pPageEntry)
	{
		// If the page entry doesn't exist, return false to tell the caller
		return false;
	}
	
	uint32_t memFrame = *pPageEntry & PAGE_BIT_ADDRESS_MASK;
	if (!(*pPageEntry & PAGE_BIT_MMIO))
		MpClearFrame(memFrame);
	
	// Remove it!!!
	*pPageEntry = 0;
	MmInvalidateSinglePage(address);
	
	return true;
}

// Checks if a specified set of mapping parameters is valid.
bool MuAreMappingParmsValid(uintptr_t start, size_t nPages)
{
	// If the size is bigger than 2 GB, there's no way we're able to map this.
	if (nPages >= 0x80000000)
		return false;
	
	// If it goes beyond the user heap's 2 GB of memory space, that means that we shouldn't map there.
	if (start + nPages * PAGE_SIZE >= KERNEL_HEAP_BASE)
		return false;
	
	return true;
}

// Checks if a continuous chain of mappings is free.
bool MuIsMappingFree(UserHeap *pHeap, uintptr_t start, size_t nPages)
{
	if (!MuAreMappingParmsValid (start, nPages))
		return false;
	
	for (size_t i = 0; i < nPages; i++)
	{
		uint32_t* pPageEntry = MuGetPageEntryAt(pHeap, start + i * PAGE_SIZE, false);
		
		// assume that it's free if that's null
		if (pPageEntry)
		{
			if (*pPageEntry & PAGE_BIT_PRESENT)
				return false;
		}
	}
	return true;
}

// Maps a chunk of memory, and forces a hint. If pPhysicalAddresses is null, allocate NEW pages.
// Otherwise, pPhysicalAddresses is treated as an ARRAY of physical page addresses of size `numPages`.
// If `bAllowClobbering` is on, all the previous mappings will be discarded. Dangerous!
bool MuMapMemoryFixedHint(UserHeap *pHeap, uintptr_t hint, size_t numPages, uint32_t *pPhysicalAddresses, bool bReadWrite, bool bAllowClobbering, bool bIsMMIO)
{
	if (bIsMMIO && !pPhysicalAddresses)
	{
		//oh come on, you can't ask me to allocate pages that will never be freed!  :(
		bIsMMIO = false;
	}
	
	if (bAllowClobbering)
	{
		if (!MuAreMappingParmsValid(hint, numPages))
			// can't map here!
			return false;
	}
	else
	{
		if (!MuIsMappingFree(pHeap, hint, numPages))
			// can't map here!
			return false;
	}
	
	// map here.
	size_t numPagesMappedSoFar = 0;
	for (numPagesMappedSoFar = 0; numPagesMappedSoFar < numPages; numPagesMappedSoFar++)
	{
		uint32_t* pPageEntry = MuGetPageEntryAt(pHeap, hint + numPagesMappedSoFar * PAGE_SIZE, true);
		if (!pPageEntry)
			goto _rollback;
		
		// If this is going to get clobbered..
		if (*pPageEntry & PAGE_BIT_PRESENT)
		{
			if (!bAllowClobbering)
				goto _rollback;
			
			// Reset it
			MuRemoveMapping(pHeap, hint + numPagesMappedSoFar * PAGE_SIZE); 
		}
		
		if (pPhysicalAddresses)
		{
			*pPageEntry = (pPhysicalAddresses[numPagesMappedSoFar]) & PAGE_BIT_ADDRESS_MASK;
		}
		else
		{
			uint32_t FreeFrame = MpFindFreeFrame();
			
			// if out of memory, roll back the mapping
			if (FreeFrame == 0xFFFFFFFF)
				goto _rollback;
			
			MpSetFrame(FreeFrame << 12);
			
			*pPageEntry = FreeFrame << 12;
		}
		
		*pPageEntry |= PAGE_BIT_PRESENT;
		
		if (bReadWrite)
			*pPageEntry |= PAGE_BIT_READWRITE;
		
		if (bIsMMIO)
			*pPageEntry |= PAGE_BIT_MMIO;
		
		MmInvalidateSinglePage(hint + numPagesMappedSoFar * PAGE_SIZE);
	}
	
	// Seems to have succeeded. Return true before we try to roll back.
	return true;
	
_rollback:
	// Roll back our changes in case something went wrong during the mapping process
	for (size_t i = 0; i < numPagesMappedSoFar; i++)
	{
		MuRemoveMapping(pHeap, hint + i * PAGE_SIZE);
	}
	
	return false;
}

uintptr_t MuFindPlaceAroundHint(UserHeap *pHeap, uintptr_t hint, size_t numPages)
{
	//OPTIMIZE oh come on, optimize this - iProgramInCpp
	
	//start from the `hint` address
	uintptr_t end = KERNEL_HEAP_BASE - numPages;
	for (uintptr_t searchHead = hint; searchHead < end; searchHead++)
	{
		if (MuIsMappingFree(pHeap, searchHead, numPages))
			return searchHead;
	}
	
	//start from the user heap base address
	for (uintptr_t searchHead = USER_HEAP_BASE; searchHead < hint; searchHead++)
	{
		if (MuIsMappingFree(pHeap, searchHead, numPages))
			return searchHead;
	}
	
	//No space left? hrm.
	return 0;
}

// Maps a chunk of memory with a hint address, but not fixed, so it can diverge a little bit.
bool MuMapMemoryNonFixedHint(UserHeap *pHeap, uintptr_t hint, size_t numPages, uint32_t *pPhysicalAddresses, void** pAddressOut, bool bReadWrite, bool bIsMMIO)
{
	// find a place to map, based on the hint
	uintptr_t newHint = MuFindPlaceAroundHint(pHeap, hint, numPages);
	
	if (newHint == 0) return false;
	
	*pAddressOut = (void*)newHint;
	
	return MuMapMemoryFixedHint(pHeap, newHint, numPages, pPhysicalAddresses, bReadWrite, false, bIsMMIO);
}

// Maps a chunk a memory without a hint address.
bool MuMapMemory(UserHeap *pHeap, size_t numPages, uint32_t* pPhysicalAddresses, void** pAddressOut, bool bReadWrite, bool bIsMMIO)
{
	void* address;
	
	bool bResult = MuMapMemoryNonFixedHint(pHeap, pHeap->m_nMappingHint, numPages, pPhysicalAddresses, &address, bReadWrite, bIsMMIO);
	if (!bResult)
		return false;
	
	// update the hint
	pHeap->m_nMappingHint = (uintptr_t)address + PAGE_SIZE * numPages;
	
	if (pHeap->m_nMappingHint >= KERNEL_HEAP_BASE)
		pHeap->m_nMappingHint  = USER_HEAP_BASE;
	
	*pAddressOut = address;
	
	return true;
}

// Unmaps a chunk of memory that has been mapped.

// Allows usage of the user heap. Changes CR3 to the user heap's page directory.
void MuUseHeap (UserHeap* pHeap)
{
	if (g_pCurrentUserHeap)
		MuResetHeap();
	
	g_pCurrentUserHeap = pHeap;
	
	MmUsePageDirectory((uintptr_t)pHeap->m_nPageDirectory);
}

// Stop using the user heap, switch to the basic kernel heap.
void MuResetHeap()
{
	g_pCurrentUserHeap = NULL;
	
	MmUsePageDirectory((uintptr_t)g_KernelPageDirectory - KERNEL_BASE_ADDRESS);
}