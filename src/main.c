#include <main.h>
#include <memory.h>
#include <print.h>
#include <idt.h>
#include <keyboard.h>
#include <elf.h>
#include <multiboot.h>

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
	
	MpInitialize(mbi);
	MhInitialize();
	
	KeIdtInit();
	
	//print the hello text, to see if the os booted properly
	LogMsg("NanoShell Experimental Operating System " VersionString);
	
	
	
	
	KeStopSystem();
}