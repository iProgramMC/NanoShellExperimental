
section .data

global e_placement
global e_frameBitsetSize
global e_frameBitsetVirt
global g_pageTableArray
global e_temporary1
global e_temporary2

; WORK: Change this if necessary.  Paging is not setup at this stage
;       so this address is purely PHYSICAL.
e_placement dd 0x400000

section .bss

	align 4096
	
	
g_pageTableArray:
	resd 2048
	
e_temporary1 resd 1
e_temporary2 resd 1
e_frameBitsetVirt resd 1
e_frameBitsetSize resd 1