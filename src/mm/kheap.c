//  ***************************************************************
//  mm/kheap.c - Creation date: 11/08/2022
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2022 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

// Namespace: Mh (Memory manager, kernel Heap)

#include <memory.h>
#include <string.h>

// Can allocate up to 256 MB of RAM.  No need for more I think,
// but if there is a need, just increase this. Recommend a Power of 2
#define C_MAX_KERNEL_HEAP_PAGE_ENTRIES 65536

uint32_t  g_KernelPageEntries  [C_MAX_KERNEL_HEAP_PAGE_ENTRIES] __attribute__((aligned(PAGE_SIZE)));

// Each element corresponds to a page entry inside the above array. It indicates how many blocks to free
// after the one that's being referenced.
// Example: if g_KernelHeapAllocSize[3] == 6, then the allocation at 3 is 7 pages long.
int       g_KernelHeapAllocSize[C_MAX_KERNEL_HEAP_PAGE_ENTRIES];

// Reserve 1024 page directory entries.

// The kernel page directory array (g_KernelPageDirectory) is directly referenced inside CR3.
// It contains page directory entries, which point to page tables.
// If a kernel page directory entry is present, it needn't necessarily have a virtual counterpart.
// If it doesn't, that means that it's been mapped since the start and shouldn't be modified.

uint32_t  g_KernelPageDirectory        [PAGE_SIZE / sizeof(uint32_t)] __attribute__((aligned(PAGE_SIZE)));
uint32_t* g_KernelPageDirectoryVirtual [PAGE_SIZE / sizeof(uint32_t)];

uint32_t* MhGetPageEntry(uintptr_t address)
{
	if (address < KERNEL_HEAP_BASE || address >= KERNEL_BASE_ADDRESS)
		return NULL;
	
	// Turn this into an index into g_KernelPageEntries.
	uint32_t pAddr = (uint32_t)address;
	uint32_t index = (pAddr - KERNEL_HEAP_BASE) >> 12;
	
	if (index <= C_MAX_KERNEL_HEAP_PAGE_ENTRIES)
		return &g_KernelPageEntries[index];
	
	return NULL;
}

void MmTlbInvalidate()
{
	__asm__ volatile ("movl %%cr3, %%ecx\n\tmovl %%ecx, %%cr3\n\t":::"cx");
}

void MmUsePageDirectory(uintptr_t pageDir)
{
	__asm__ volatile ("mov %0, %%cr3"::"r"((uint32_t*)pageDir));
}

void MmInvalidateSinglePage(uintptr_t add)
{
	__asm__ volatile ("invlpg (%0)\n\t"::"r"(add):"memory");
}

void MhInitialize()
{
	for (uint32_t i = 0; i < C_MAX_KERNEL_HEAP_PAGE_ENTRIES; i++)
	{
		g_KernelPageEntries  [i] = 0;
		g_KernelHeapAllocSize[i] = 0;
	}
	
	// Map the kernel heap's pages starting at 0x80000000.
	uint32_t pageDirIndex = KERNEL_HEAP_BASE / PAGE_SIZE / PAGE_SIZE * 4;
	for (int i = 0; i < C_MAX_KERNEL_HEAP_PAGE_ENTRIES; i += 1024)
	{
		uint32_t pageTableAddr = (uint32_t)&g_KernelPageEntries[i];
		
		g_KernelPageDirectory[pageDirIndex] = (pageTableAddr - KERNEL_BASE_ADDRESS) | PAGE_BIT_READWRITE | PAGE_BIT_PRESENT;
		pageDirIndex++;
	}
	
	MmTlbInvalidate();
}

void* MhSetupPage(int index, uint32_t* pPhysOut)
{
	// if we require a physical address, allocate a physical page right away
	if (pPhysOut)
	{
		uintptr_t frame = MpRequestFrame();
		if (!frame)
		{
			LogMsg("Out of memory in MhSetupPage?!");
			return NULL;
		}
		
		// Mark this page entry as present, and return its address.
		g_KernelPageEntries[index] = frame | PAGE_BIT_PRESENT | PAGE_BIT_READWRITE | PAGE_BIT_USERSUPER;
		*pPhysOut = frame;
	}
	else
	{
		//defer the allocation to when it's needed
		g_KernelPageEntries[index] = PAGE_BIT_DAI | PAGE_BIT_READWRITE | PAGE_BIT_USERSUPER;
	}
	
	g_KernelHeapAllocSize[index] = 0;
	
	uintptr_t returnAddr = (KERNEL_HEAP_BASE + (index << 12));
	MmInvalidateSinglePage(returnAddr);
	
	return (void*)returnAddr;
}

void* MhAllocateSinglePage(uint32_t* pPhysOut)
{
	// Find a free page frame.
	for (int i = 0; i < C_MAX_KERNEL_HEAP_PAGE_ENTRIES; i++)
	{
		if (!(g_KernelPageEntries[i] & PAGE_BIT_PRESENT))
		{
			return MhSetupPage(i, pPhysOut);
		}
	}
	
	// Out of memory.
	return NULL;
}

void MhFreePage(void* pPage)
{
	if (!pPage) return;
	
	// Turn this into an index into g_KernelPageEntries.
	uint32_t pAddr = (uint32_t)pPage;
	
	uint32_t index = (pAddr - KERNEL_HEAP_BASE) >> 12;
	
	// don't free a page if it was marked as demand-paged but was never actually demanded
	if (g_KernelPageEntries[index] & PAGE_BIT_PRESENT)
	{
		// Get the old physical address. We want to remove the frame.
		uint32_t physicalFrame = g_KernelPageEntries[index] & PAGE_BIT_ADDRESS_MASK;
		
		MpClearFrame(physicalFrame);
		
		MmInvalidateSinglePage((uintptr_t)pPage);
	}
	
	// Ok. Free it now
	g_KernelPageEntries  [index] = 0;
	g_KernelHeapAllocSize[index] = 0;
}

void MhFree(void* pPage)
{
	if (!pPage) return;
	
	// Turn this into an index into g_KernelPageEntries.
	uint32_t pAddr = (uint32_t)pPage;
	
	uint32_t index = (pAddr - KERNEL_HEAP_BASE) >> 12;
	
	int nSubsequentAllocs = g_KernelHeapAllocSize[index];
	
	MhFreePage(pPage);
	
	pAddr += PAGE_SIZE;
	for (int i = 0; i < nSubsequentAllocs; i++)
	{
		MhFreePage((void*)pAddr);
		pAddr += PAGE_SIZE;
	}
}
void* MhAllocate(size_t size, uint32_t* pPhysOut)
{
	if (size <= PAGE_SIZE)
	{
		return MhAllocateSinglePage(pPhysOut);
	}
	else
	{
		//more than one page, take matters into our own hands:
		int numPagesNeeded = ((size - 1) >> 12) + 1;
		//ex: if we wanted 6100 bytes, we'd take 6100-1=6099, then divide that by 4096 (we get 1) and add 1
		//    if we wanted 8192 bytes, we'd take 8192-1=8191, then divide that by 4096 (we get 1) and add 1 to get 2 pages
		
		for (int i = 0; i < C_MAX_KERNEL_HEAP_PAGE_ENTRIES; i++)
		{
			// A non-allocated pageframe?
			if (!(g_KernelPageEntries[i] & PAGE_BIT_PRESENT))
			{
				// Yes.  Are there at least numPagesNeeded holes?
				int jfinal = i + numPagesNeeded;
				for (int j = i; j < jfinal; j++)
				{
					//Are there any already taken pages before we reach the end.
					if (g_KernelPageEntries[j] & PAGE_BIT_PRESENT)
					{
						//Yes.  This hole isn't large enough.
						i = j;
						goto _label_continue;
					}
				}
				// Nope! We have space here!  Let's map all the pages, and return the address of the first one.
				//LogMsg("Setting up page number %d", i);
				void* pointer = MhSetupPage(i, pPhysOut);
				
				// Not to forget, set the memory allocation size below:
				g_KernelHeapAllocSize[i] = numPagesNeeded - 1;
				
				for (int j = i + 1, k = 1; j < jfinal; j++, k++)
				{
					//LogMsg("Setting up page number %d", j);
					uint32_t* pPhysOutNew = pPhysOut;
					if (pPhysOutNew)
						pPhysOutNew += k;
					MhSetupPage (j, pPhysOutNew);
				}
				
				return pointer;
			}
		_label_continue:;
		}
		
		//no continuous addressed pages are left. :^(
		return NULL;
	}
}
void* MhReAllocate(void *oldPtr, size_t newSize)
{
	// step 1: If the pointer is null, just allocate a new array of size `size`.
	if (!oldPtr)
		return MhAllocate(newSize, NULL);
	
	// step 2: get the first page's number
	uint32_t pAddr = (uint32_t)oldPtr;
	
	uint32_t index = (pAddr - KERNEL_HEAP_BASE) >> 12;
	
	// step 3: figure out the old size of this block.
	int* pSubsequentAllocs = &g_KernelHeapAllocSize[index];
	
	size_t oldSize = PAGE_SIZE * (*pSubsequentAllocs + 1);
	
	// If the provided size is smaller, return the same block, but shrunk.
	if (oldSize >= newSize)
	{
		size_t numPagesNeeded =  ((newSize - 1) >> 12) + 1;
		
		int oldPages = *pSubsequentAllocs  + 1;
		*pSubsequentAllocs = numPagesNeeded - 1;
		
		// where freeing pages starts:
		uint8_t* addr = (uint8_t*)oldPtr + numPagesNeeded * PAGE_SIZE;
		
		for (int i = numPagesNeeded; i < oldPages; i++)
		{
			MhFreePage(addr);
			addr += PAGE_SIZE;
		}
		
		return oldPtr;
	}
	else
	{
		// Check if there are this many free blocks available at all.
		size_t numPagesNeeded =  ((newSize - 1) >> 12) + 1;
		int oldPages = *pSubsequentAllocs  + 1;
		
		bool spaceAvailable = true;
		
		for (size_t i = oldPages; i < numPagesNeeded; i++)
		{
			if (g_KernelPageEntries[index + i] & PAGE_BIT_PRESENT)
			{
				spaceAvailable = false;
				break;
			}
		}
		
		if (spaceAvailable)
		{
			// Proceed with the expansion.
			
			for (size_t i = oldPages; i < numPagesNeeded; i++)
			{
				void *ptr = MhSetupPage (index + i, NULL);
				
				// if not null, increment by 4.
				if (!ptr)
				{
					// All, hell naw
					//TODO: rollback
					ASSERT(!"TODO: Roll back from failed MmReAllocate?");
					return NULL;
				}
			}
			
			*pSubsequentAllocs = numPagesNeeded - 1;
			
			return oldPtr;
		}
	}
	
	// If nothing else works, just allocate and memcpy().
	
	void* newPtr = MhAllocate(newSize, NULL);
	if (!newPtr)
		return NULL;
	
	size_t minSize = newSize;
	if (minSize > oldSize)
		minSize = oldSize;
	memcpy(newPtr, oldPtr, minSize);
	
	MhFree(oldPtr);
	
	return newPtr;
}
