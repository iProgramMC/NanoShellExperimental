
; main multiboot header
section .multibootdata
	;multiboot V1 spec
	align 4
	dd 0x1BADB002
	%ifdef vbe
		dd 0x06
		dd - (0x1BADB002 + 0x06)
	%else
		dd 0x02
		dd - (0x1BADB002 + 0x02)
	%endif
	dd 0
	dd 0
	dd 0
	dd 0
	dd 0
	%ifdef vbe
	dd 0
	dd 1024
	dd 768
	dd 32
	%else
	dd 0
	dd 0
	dd 0
	dd 0
	%endif