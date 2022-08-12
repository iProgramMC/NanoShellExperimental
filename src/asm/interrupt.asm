BITS 32
SECTION .text


extern IrqTimer
extern IrqKeyboard
extern OnSyscallReceived
extern IsrSoftware
global IrqTimerA
global OnSyscallReceivedA
IrqTimerA:
	pusha
	call IrqTimer
	popa
	iretd
global IrqKeyboardA
IrqKeyboardA:
	pusha
	call IrqKeyboard
	popa
	iretd
global IsrSoftwareA
IsrSoftwareA:
	pusha
	call IsrSoftware
	popa
	iretd
global OnSyscallReceivedA
OnSyscallReceivedA:
	cli
	push 0
	pusha
	
	; call isrSoftware with our one and only parm:
	push esp
	call OnSyscallReceived
	add esp, 4
	
	popa
	add esp, 4 ;remove the zero from the stack
	sti
	iretd
	
; Special handler for interrupt 0xE (page fault)
global IsrStub14
extern MmOnPageFault
IsrStub14:
	; the error code has already been pushed
	pusha                         ; back up all registers
	mov eax, cr2                  ; push cr2 (the faulting address), to complete the 'registers' struct
	push eax
	push esp                      ; push esp - a pointer to the registers* struct
	call MmOnPageFault            ; call the page fault handler
	add  esp, 8                   ; pop away esp and cr2
	popa                          ; restore the registers, then return from the page fault
	add  esp, 4                   ; pop away the error code
	iretd
	