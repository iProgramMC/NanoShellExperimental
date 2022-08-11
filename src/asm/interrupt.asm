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
	
	