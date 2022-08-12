#ifndef _IDT_H
#define _IDT_H

typedef struct {
	unsigned short offset_lowerbits;
	unsigned short int selector;
	unsigned char zero;
	unsigned char type_attr;
	unsigned short offset_higherbits;
}
__attribute__((packed)) 
IdtEntry;

typedef struct
{
	uint16_t limit;
	uint32_t base;
}
__attribute__((packed))
IdtPointer;

/**
 * USAGE FOR THIS STRUCT:
 * When you get an interrupt (which pushes eflags, cs, and eip to the stack),
 * IMMEDIATELY push some error code (because our fault handlers need it, and
 * I want to make this as generic as possible). Then push ESP, and call the C
 * handler. (it'll be defined like `void someHandler(Registers* pRegs);`)
 *
 * Then when we return from the function, immediately `add esp, 4`, then do a `popa`
 * and `add esp, 4` again.
 * 
 * NOTE: Fault handlers are not supposed to return.  If they do anyway *DON'T*
 * just iretd out of them, they will be re-called (at best), or a different exception
 * will be thrown (at worst), because the EIP will be invalid (error code would replace it)
*/

typedef struct 
{
	uint32_t//:
		cr2,
		edi, esi, ebp, esp, ebx, edx, ecx, eax,
		error_code, 
		eip, cs, eflags;
}
Registers;

extern void KeIdtInit();
extern void KeTimerInit();
extern void IrqKeyboardA(void);
extern void IrqTimerA(void);
extern void KeIdtLoad(IdtPointer *idt_ptr);

#endif//_IDT_H