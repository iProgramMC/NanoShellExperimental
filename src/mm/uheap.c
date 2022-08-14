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

UserHeap* MuGetCurrentHeap()
{
	return g_pCurrentUserHeap;
}

extern uint32_t g_KernelPageDirectory[];

// Creates a new (empty) heap structure.
UserHeap* MuCreateHeap()
{
	UserHeap *pHeap = MhAllocate(sizeof(UserHeap), NULL);
	
	pHeap->m_nRefCount       = 1; // One reference. When this heap gets cloned, its reference count is increased.
	pHeap->m_nPageDirectory  = 0;
	pHeap->m_nMappingHint    = USER_HEAP_BASE;
	pHeap->m_pPageDirectory  = MhAllocateSinglePage(&pHeap->m_nPageDirectory);
	pHeap->m_pAllocations    = NULL;
	pHeap->m_pUserHeapParent = NULL;
	
	memset(pHeap->m_pPageDirectory, 0, PAGE_SIZE);
	
	// Copy the latter half from the kernel heap.
	for (int i = 0x200; i < 0x400; i++)
		pHeap->m_pPageDirectory[i] = g_KernelPageDirectory[i];
	
	memset(pHeap->m_pPageTables, 0, sizeof(pHeap->m_pPageTables));
	
	return pHeap;
}

void MuFreeHeapAllocChain(UserHeapAllocChainItem* pChain)
{
	while (pChain)
	{
		UserHeapAllocChainItem* pThis = pChain;
		pChain = pChain->pNext;
		MhFree(pThis);
	}
}

// This method of keeping track of mappings is kind of naive, in that we use a linear search rather than a binary search, this could get messy quickly

// Adds a mapping to the heap's list of mappings. Allows us to use munmap() and only specify the starting address of the mapping.
void MuAddMappingToHeapsList(UserHeap *pHeap, void *pStart, size_t numPages)
{
	// If the heap list is NULL, start off with one chain element
	if (!pHeap->m_pAllocations)
	{
		pHeap->m_pAllocations = MhAllocateSinglePage(NULL);
		memset (pHeap->m_pAllocations, 0, PAGE_SIZE);
	}
	
	// Look through the allocation table and see what allocation spot is free
	UserHeapAllocChainItem *pItem = pHeap->m_pAllocations;
	
	if (!pItem)
	{
		// Hrm. But we just allocated something, mayhaps it didn't work?
		LogMsg("pHeap->m_pAllocations allocation didn't work (%s:%d)", __FILE__, __LINE__);
		KeStopSystem();
	}
	
	while (true)
	{
		// 511 on i686, 255 on x86_64. Don't actually care about x86_64, just wanted to point it out
		for (size_t i = 0; i < ARRAY_COUNT (pItem->m_allocs); i++)
		{
			if (pItem->m_allocs[i].m_pAddr == NULL)
			{
				// we've found a free spot!!
				pItem->m_allocs[i].m_pAddr  = pStart;
				pItem->m_allocs[i].m_nPages = numPages;
				return;
			}
		}
		
		if (!pItem->pNext) break; //we'll use the last element anyway
		
		pItem = pItem->pNext;
	}
	
	// We have NOT found a new spot... let's allocate one!
	UserHeapAllocChainItem* pNewItem = MhAllocateSinglePage(NULL);
	memset (pNewItem, 0, PAGE_SIZE);
	
	pItem->pNext = pNewItem;
	
	// Now, use the first slot (guaranteedly empty) to put our new allocation in
	pNewItem->m_allocs[0].m_pAddr  = pStart;
	pNewItem->m_allocs[0].m_nPages = numPages;
}

bool MuIsMappingOnHeapsList(UserHeap *pHeap, void *pStart, UserHeapAllocChainItem** pOutItem, int *pOutIndex)
{
	// Look through the allocation table and see what allocation spot has it
	UserHeapAllocChainItem *pItem = pHeap->m_pAllocations;
	
	if (pOutItem)  *pOutItem  = NULL;
	if (pOutIndex) *pOutIndex = 0;
	
	if (!pItem)
	{
		//We have no logged allocations, I guess?
		return false;
	}
	
	while (pItem)
	{
		for (size_t i = 0; i < ARRAY_COUNT (pItem->m_allocs); i++)
		{
			if (pItem->m_allocs[i].m_pAddr == pStart)
			{
				// It is on The List.
				if (pOutItem)  *pOutItem  = pItem;
				if (pOutIndex) *pOutIndex = i;
				return true;
			}
		}
		
		pItem = pItem->pNext;
	}
	
	// No, it's not on The List
	return false;
}

void MuDebugLogHeapList(UserHeap *pHeap)
{
	UserHeapAllocChainItem *pItem = pHeap->m_pAllocations;
	
	while (pItem)
	{
		for (size_t i = 0; i < ARRAY_COUNT (pItem->m_allocs); i++)
		{
			if (pItem->m_allocs[i].m_pAddr)
			{
				LogMsg("Allocation: %p. Size in pages: %d (%x)", pItem->m_allocs[i].m_pAddr, pItem->m_allocs[i].m_nPages, pItem->m_allocs[i].m_nPages);
			}
		}
		
		pItem = pItem->pNext;
	}
}

// An automatic version. Won't use this in MuUnMap because we output stuff already in MuIsMappingOnHeapsList
bool MuRemoveMappingFromList(UserHeap *pHeap, void *pStart)
{
	UserHeapAllocChainItem* pItem = NULL; int nIndex = 0;
	
	if (!MuIsMappingOnHeapsList(pHeap, pStart, &pItem, &nIndex))
	{
		LogMsg("Could not remove mapping started at %p", pStart);
		return false;
	}
	
	pItem->m_allocs[nIndex].m_pAddr  = NULL;
	pItem->m_allocs[nIndex].m_nPages = 0;
	
	return true;
}

// Creates a new structure and copies all of the pages of the previous one. Uses COW to achieve this.
// (Unfortunately, this won't work on the i386 itself, but works on i486 and up, and I'm not sure we
//  can get this running on an i386 without some hacks to get around the lack of a boot loader)
// TODO: Allow a compiler switch to disable CoW in case something weird was found that makes it impossible
UserHeap* MuCloneHeap(UserHeap* pHeapToClone)
{
	UserHeap *pHeap = MuCreateHeap();
	
	pHeapToClone->m_nRefCount++;
	pHeap->m_pUserHeapParent = pHeapToClone;
	
	// Clone each page table
	for (int i = 0x000; i < 0x200; i++)
	{
		if (pHeapToClone->m_pPageTables[i])
		{
			MuCreatePageTable(pHeap, i);
			
			for (int entry = 0x000; entry < 0x400; entry++)
			{
				uint32_t* pEntryDst = MuGetPageEntryAt(pHeap,        i << 22 | entry << 12, true);
				uint32_t* pEntrySrc = MuGetPageEntryAt(pHeapToClone, i << 22 | entry << 12, true);
				
				if (*pEntrySrc & PAGE_BIT_PRESENT)
				{
					//We don't have a way to track the reference count of individual pages.
					//So when the parent gets killed, the children will be working with a
					//freed page, which could potentially be overwritten, and uh oh.
					
					//This is fixed by keeping the parent alive until all the children have been killed too
					
					//ensure that the bit is clear
					
					// MMIO is inilligible for CoW
					if (*pEntrySrc & PAGE_BIT_MMIO)
					{
						// Make the second page also point to the MMIO stuff
						*pEntryDst = *pEntrySrc;
					}
					else if (*pEntrySrc & PAGE_BIT_DAI)
					{
						// Make the second page be demand-paging too
						*pEntryDst = *pEntrySrc;
					}
					else
					{
						*pEntryDst &= ~PAGE_BIT_READWRITE;
						
						//use the same physical page as the source
						*pEntryDst = (*pEntrySrc & PAGE_BIT_ADDRESS_MASK) | PAGE_BIT_PRESENT;
						
						//if it's read-write, set the COW bit
						if (*pEntrySrc & PAGE_BIT_READWRITE)
						{
							*pEntryDst |= PAGE_BIT_COW;
						}
					}
				}
			}
		}
	}
	
	return pHeap;
}

void MuKillPageTablesEntries(PageTable* pPageTable)
{
	// Free each of the pages, if there are any
	for (int i = 0; i < 0x400; i++)
	{
		if (pPageTable->m_pageEntries[i] & PAGE_BIT_PRESENT)
		{
			uint32_t frame = pPageTable->m_pageEntries[i] & PAGE_BIT_ADDRESS_MASK;
			
			MpClearFrame(frame);
			
			pPageTable->m_pageEntries[i] = 0;
		}
	}
}

void MuKillHeap(UserHeap* pHeap)
{
	pHeap->m_nRefCount--;
	if (pHeap->m_nRefCount < 0)
	{
		// Uh oh
		LogMsg("Heap %p has reference count %d, uh oh", pHeap, pHeap->m_nRefCount);
		KeStopSystem();
	}
	
	if (pHeap->m_nRefCount == 0)
	{
		LogMsg("Heap %p will be killed off now.", pHeap);
		// To kill the heap, first we need to empty all the page tables
		for (int i = 0; i < 0x200; i++)
		{
			if (pHeap->m_pPageTables[i])
			{
				MuRemovePageTable(pHeap, i);
			}
		}
		
		// Kill off the parent as well, if needed
		if (pHeap->m_pUserHeapParent)
			MuKillHeap(pHeap->m_pUserHeapParent);
		
		// Should be good to go.  Free the page directory, and then the heap itself
		MuFreeHeapAllocChain(pHeap->m_pAllocations);
		
		MhFree(pHeap->m_pPageDirectory);
		MhFree(pHeap);
	}
	else
	{
		LogMsg("Heap %p won't die yet, because it's reference count is %d!", pHeap, pHeap->m_nRefCount);
	}
}

void MuCreatePageTable(UserHeap *pHeap, int pageTable)
{
	// Reset the page table there.
	pHeap->m_pPageDirectory[pageTable] = 0;
	
	// Create a new page table page on the kernel heap.
	uint32_t physOut[2];//don't actually care about the second page, but we need it
	
	pHeap->m_pPageTables[pageTable] = MhAllocate(PAGE_SIZE * 2, physOut);
	
	pHeap->m_pPageDirectory[pageTable] = physOut[0];
	
	memset(pHeap->m_pPageTables[pageTable], 0, PAGE_SIZE * 2);
	
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
	uint32_t* pPageEntry = &pHeap->m_pPageTables[addressSplit.pageTable]->m_pageEntries[addressSplit.pageEntry];
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
	if (*pPageEntry & (PAGE_BIT_PRESENT | PAGE_BIT_DAI))
	{
		// Yeah... For obvious reasons we can't map here
		return false;
	}
	
	if (physAddress)
	{
		*pPageEntry = physAddress & PAGE_BIT_ADDRESS_MASK;
		*pPageEntry |= PAGE_BIT_PRESENT;
		if (bReadWrite)
			*pPageEntry |= PAGE_BIT_READWRITE;
	}
	else
	{
		*pPageEntry = PAGE_BIT_DAI;
	}
	
	// WORK: This might not actually be needed if TLB doesn't actually cache non-present pages. Better to be on the safe side, though
	MmInvalidateSinglePage(address);
	
	MuAddMappingToHeapsList(pHeap, (void*)address, 1);
	
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
	
	// A page entry was allocated here, and it has been copied-on-write
	if ((*pPageEntry & PAGE_BIT_PRESENT) && !(*pPageEntry & PAGE_BIT_COW))
	{
		uint32_t memFrame = *pPageEntry & PAGE_BIT_ADDRESS_MASK;
		
		if (!(*pPageEntry & PAGE_BIT_MMIO))
			MpClearFrame(memFrame);
		
		// Remove it!!!
		*pPageEntry = 0;
		MmInvalidateSinglePage(address);
	}
	
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
		if (*pPageEntry & (PAGE_BIT_PRESENT | PAGE_BIT_DAI))
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
			/*
			uint32_t FreeFrame = MpFindFreeFrame();
			
			// if out of memory, roll back the mapping
			if (FreeFrame == 0xFFFFFFFF)
				goto _rollback;
			
			MpSetFrame(FreeFrame << 12);
			
			*pPageEntry = FreeFrame << 12;*/
		}
		
		//*pPageEntry |= PAGE_BIT_PRESENT;
		*pPageEntry |= PAGE_BIT_DAI;
		
		if (bReadWrite)
			*pPageEntry |= PAGE_BIT_READWRITE;
		
		if (bIsMMIO)
			*pPageEntry |= PAGE_BIT_MMIO;
		
		MmInvalidateSinglePage(hint + numPagesMappedSoFar * PAGE_SIZE);
	}
	
	// Seems to have succeeded. Add it to The (mapping) List.
	
	MuAddMappingToHeapsList(pHeap, (void*)hint, numPages);
	
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
	
	if (!pHeap) return;
	
	MmUsePageDirectory((uintptr_t)pHeap->m_nPageDirectory);
}

// Stop using the user heap, switch to the basic kernel heap.
void MuResetHeap()
{
	g_pCurrentUserHeap = NULL;
	
	MmUsePageDirectory((uintptr_t)g_KernelPageDirectory - KERNEL_BASE_ADDRESS);
}