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

#define PAGE_BIT_ADDRESS_MASK (0xFFFFF000)

#include <main.h>
#include <multiboot.h>

// Physical memory manager
uint32_t MpFindFreeFrame();
void MpSetFrame  (uint32_t frameAddr);
void MpClearFrame(uint32_t frameAddr);
void MpInitialize(multiboot_info_t* pInfo);

// Kernel heap

void* MhAllocateSinglePage(uint32_t* pPhysOut);
void  MhFreePage(void* pPage);
void  MhFree(void* pPage);
void  MhInitialize();

#endif//_MEMORY_H