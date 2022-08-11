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

void MmTlbInvalidate()
{
	__asm__ volatile ("movl %%cr3, %%ecx\n\tmovl %%ecx, %%cr3\n\t":::"cx");
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
	uint32_t frame = MpFindFreeFrame();
	if (frame == 0xFFFFFFFFu)
	{
		// Out of memory.
		return NULL;
	}
	
	MpSetFrame(frame << 12);
	
	// Mark this page entry as present, and return its address.
	g_KernelPageEntries  [index] = frame << 12 | PAGE_BIT_PRESENT | PAGE_BIT_READWRITE | PAGE_BIT_USERSUPER;
	g_KernelHeapAllocSize[index] = 0;
	
	if (pPhysOut)
		*pPhysOut = frame << 12;
	
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
	
	// Get the old physical address. We want to remove the frame.
	uint32_t physicalFrame = g_KernelPageEntries[index] & PAGE_BIT_ADDRESS_MASK;
	
	// Ok. Free it now
	g_KernelPageEntries  [index] = 0;
	g_KernelHeapAllocSize[index] = 0;
	
	MpClearFrame(physicalFrame);
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

