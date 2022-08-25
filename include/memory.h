//  ***************************************************************
//  memory.h - Creation date: 11/08/2022
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2022 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************

#ifndef _MEMORY_H
#define _MEMORY_H

#define KERNEL_BASE_ADDRESS (0xC0000000)
#define KERNEL_HEAP_BASE    (0x80000000)
#define USER_HEAP_BASE      (0x40000000)
#define PAGE_SIZE           (0x1000)

#define PAGE_BIT_PRESENT      (0x1)
#define PAGE_BIT_READWRITE    (0x2)
#define PAGE_BIT_USERSUPER    (0x4)
#define PAGE_BIT_WRITETHRU    (0x8)
#define PAGE_BIT_CACHEDISABLE (0x10)
#define PAGE_BIT_ACCESSED     (0x20)

// Page bits that the kernel uses, and are marked as 'available' by the spec
#define PAGE_BIT_MMIO         (0x800) // (1 << 11). If this is MMIO, set this bit to let the kernel know that this shouldn't be unmapped
#define PAGE_BIT_COW          (0x400) // (1 << 10). Copy On Write. If a fault occurs here, copy the specified page.
#define PAGE_BIT_DAI          (0x200) // (1 <<  9). Don't Allocate Instantly. If a fault occurs here, allocate a physical page.

#define PAGE_BIT_ADDRESS_MASK (0xFFFFF000)

#include <main.h>
#include <multiboot.h>
#include <lock.h>
#include <idt.h>

// Namespace guide:
// * Mu = User Heaps
// * Mh = Kernel Heap
// * Mp = Physical memory manager
// * Mr = Physical memory Reference counter manager
// * Mm = Exposed programming interface

typedef struct
{
	uint32_t m_refCounts[PAGE_SIZE / sizeof(uint32_t)];
}
RefCountTableLevel1;

typedef struct
{
	RefCountTableLevel1* m_level1[PAGE_SIZE / sizeof(uint32_t)];
}
RefCountTableLevel0;

typedef struct
{
	uint32_t m_pageEntries[PAGE_SIZE / 4];
}
PageTable;

// User heap structure
typedef struct UserHeap
{
	SafeLock   m_lock;             // The lock associated with this heap object
	
	int        m_nRefCount;        // The reference count. If it goes to zero, this gets killed
	
	uint32_t*  m_pPageDirectory;   // The virtual  address of the page directory
	uint32_t   m_nPageDirectory;   // The physical address of the page directory
	PageTable* m_pPageTables[512]; // The user half's page tables referenced by the page directory.
	uint32_t   m_nMappingHint;     // The hint to use when mapping with no hint next.
}
UserHeap;

// Physical memory manager
uint32_t MpFindFreeFrame();
void MpSetFrame  (uint32_t frameAddr);
void MpClearFrame(uint32_t frameAddr);
void MpInitialize(multiboot_info_t* pInfo);
int  MpGetNumFreePages();
uintptr_t MpRequestFrame(bool bIsKernelHeap);

// Physical memory reference count manager
uint32_t MrGetReferenceCount(uintptr_t page);
uint32_t MrReferencePage(uintptr_t page);
uint32_t MrUnreferencePage(uintptr_t page);

// Hardware
void MmTlbInvalidate();
void MmUsePageDirectory(uintptr_t pageDir);   //unsafe!! This is exposed just so that other memory code can use it.
void MmInvalidateSinglePage(uintptr_t add);
void MmOnPageFault(Registers* pRegs);
int  MmGetNumPageFaults();

// Kernel heap
void* MhAllocate(size_t size, uint32_t* pPhysOut);
void* MhReAllocate(void *oldPtr, size_t newSize);
void* MhAllocateSinglePage(uint32_t* pPhysOut);
void  MhFreePage(void* pPage);
void  MhFree(void* pPage);
void  MhInitialize();
uint32_t* MhGetPageEntry(uintptr_t address);

// User heap manager
void MuUseHeap (UserHeap* pHeap);
void MuResetHeap();
UserHeap* MuCreateHeap();
UserHeap* MuGetCurrentHeap();
UserHeap* MuCloneHeap(UserHeap* pHeapToClone);
void MuKillHeap(UserHeap *pHeap);
uint32_t* MuGetPageEntryAt(UserHeap* pHeap, uintptr_t address, bool bGeneratePageTable);
bool MuCreateMapping(UserHeap *pHeap, uintptr_t address, uint32_t physAddress, bool bReadWrite);
bool MuAreMappingParmsValid(uintptr_t start, size_t nPages);
bool MuIsMappingFree(UserHeap *pHeap, uintptr_t start, size_t nPages);
bool MuMapMemory(UserHeap *pHeap, size_t numPages, uint32_t* pPhysicalAddresses, void** pAddressOut, bool bReadWrite, bool bIsMMIO);
bool MuMapMemoryNonFixedHint(UserHeap *pHeap, uintptr_t hint, size_t numPages, uint32_t *pPhysicalAddresses, void** pAddressOut, bool bReadWrite, bool bIsMMIO);
bool MuMapMemoryFixedHint(UserHeap *pHeap, uintptr_t hint, size_t numPages, uint32_t *pPhysicalAddresses, bool bReadWrite, bool bAllowClobbering, bool bIsMMIO);
void MuCreatePageTable(UserHeap *pHeap, int pageTable);
void MuRemovePageTable(UserHeap *pHeap, int pageTable);
bool MuRemoveMapping(UserHeap *pHeap, uintptr_t address);

#endif//_MEMORY_H