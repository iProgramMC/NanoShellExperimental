// IDT Handler
#include <main.h>
#include <idt.h>
#include <keyboard.h>
#include <syscall.h>

#define KBDATA 0x60
#define KBSTAT 0x64
#define IDT_SIZE 256
#define INTGATE 0x8e
#define KECODESEG 0x08

IdtEntry g_idt [IDT_SIZE];

bool g_interruptsAvailable = false;

void SetupSoftInterrupt (int intNum, void *pIsrHandler)
{
	IdtEntry* pEntry = &g_idt[intNum];
	pEntry->offset_lowerbits = ((int)(pIsrHandler) & 0xffff);
	pEntry->offset_higherbits = ((int)(pIsrHandler) >> 16);
	pEntry->zero = 0;
	pEntry->type_attr = INTGATE;
	pEntry->selector = KECODESEG;
}
void SetupInterrupt (uint8_t *mask1, uint8_t* mask2, int intNum, void* isrHandler)
{
	IdtEntry* pEntry = &g_idt[0x20 + intNum];
	pEntry->offset_lowerbits = ((int)(isrHandler) & 0xffff);
	pEntry->offset_higherbits = ((int)(isrHandler) >> 16);
	pEntry->zero = 0;
	pEntry->type_attr = INTGATE;
	pEntry->selector = KECODESEG;
	
	int picFlag = intNum & 7;
	int whichPic = intNum >= 8;
	
	*(whichPic ? mask2 : mask1) &= ~(1<<picFlag);
}
void SetupExceptionInterrupt (int intNum, void* isrHandler)
{
	IdtEntry* pEntry = &g_idt[intNum];
	pEntry->offset_lowerbits = ((int)(isrHandler) & 0xffff);
	pEntry->offset_higherbits = ((int)(isrHandler) >> 16);
	pEntry->zero = 0;
	pEntry->type_attr = INTGATE;
	pEntry->selector = KECODESEG;
}

void IsrStub14();

void KeTimerInit() 
{
	WritePort(0x43, 0x34); // generate frequency
	
	// set frequency
	//int pitMaxFreq = 1193182;
	
	//note that the PIT frequency divider has been hardcoded to 65535
	//for testing.
	
	int pit_frequency = 65535;//(pitMaxFreq / 10);
	WritePort(0x40, (uint8_t)( pit_frequency       & 0xff));
	WritePort(0x40, (uint8_t)((pit_frequency >> 8) & 0xff));
}
static int s_ticks = 0;
void IrqTimer()
{
	s_ticks++;
	//LogMsg("Timer!");
	WritePort(0x20, 0x20);
	WritePort(0xA0, 0x20);
}
int GetTickCount()
{
	return s_ticks;
}
void IsrSoftware()
{
	LogMsg("SYSCALL!");
}
unsigned long idtPtr[2];
extern void IsrSoftwareA();
extern void OnSyscallReceivedA();
void KeIdtLoad1(IdtPointer *ptr)
{
	__asm__ ("lidt %0" :: "m"(*ptr));
}
void KeIdtInit()
{
	uint8_t mask1 = 0xff, mask2 = 0xff;
	
	SetupInterrupt (&mask1, &mask2, 0x0, IrqTimerA);
	SetupInterrupt (&mask1, &mask2, 0x1, IrqKeyboardA);
	SetupInterrupt (&mask1, &mask2, 0x2, NULL); // IRQ2: Cascade. Never triggered
	
	SetupExceptionInterrupt (0x0E, IsrStub14);
	
	SetupSoftInterrupt (0x80, OnSyscallReceivedA);
	
	//initialize the pics
	WritePort (0x20, 0x11);
	WritePort (0xa0, 0x11);
	
	//flush the PICs
	for (int i=0; i<16; i++)
	{
		WritePort(0x20, 0x20);
		WritePort(0xA0, 0x20);
	}
	
	//set int num offsets, because fault offsets are hardcoded
	//inside the CPU. We dont want a crash triggering a keyboard
	//interrupt do we?!
	WritePort (0x21, 0x20); //main PIC
	WritePort (0xa1, 0x28); //secondary PIC, unused
	
	//no need to initialize the second PIC, but I'll still
	//do it anyway just to be safe
	WritePort (0x21, 0x04);
	WritePort (0xA1, 0x02);
	
	//something
	WritePort (0x21, 0x01);
	WritePort (0xA1, 0x01);
	
	WritePort (0x21, mask1);
	WritePort (0xA1, mask2);
	
	// Load the IDT
	
	IdtPointer ptr;
	ptr.limit = (sizeof(IdtEntry) * IDT_SIZE);
	ptr.base = (size_t)g_idt;
	
	g_interruptsAvailable = true;// sti was set in LoadIDT
	KeIdtLoad1 (&ptr);
	
	KeTimerInit();
	
	sti;
}