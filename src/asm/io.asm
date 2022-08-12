bits 32
section .text

global ReadPort
global WritePort
global ReadPortW
global WritePortW
global WriteFont8px
global WriteFont16px

extern e_placement
extern e_frameBitsetVirt

; PORT I/O
ReadPort:	
	mov edx, [esp + 4]
	in al, dx
	ret
WritePort:
	mov edx, [esp + 4]
	mov al, [esp + 8]
	out dx, al
	ret
ReadPortW:
	mov edx, [esp + 4]
	in ax, dx
	ret
WritePortW:
	mov edx, [esp + 4]
	mov ax, [esp + 8]
	out dx, ax
	ret

; Functions for DRIVERS/VGA.H
; https://wiki.osdev.org/VGA_Fonts#Get_from_VGA_RAM_directly
WriteFont8px:
	mov esi, [esp + 4]
	mov edi, 0xC00A0000
	;in: edi=4k buffer
	;out: buffer filled with font
	;clear even/odd mode
	mov	dx, 03ceh
	mov	ax, 5
	out	dx, ax
	;map VGA memory to 0A0000h
	mov	ax, 0406h
	out	dx, ax
	;set bitplane 2
	mov	dx, 03c4h
	mov	ax, 0402h
	out	dx, ax
	;clear even/odd mode (the other way, don't ask why)
	mov	ax, 0604h
	out	dx, ax
	;copy charmap
	mov	ecx, 256
	;copy 8 bytes to bitmap
.b:	
	movsd
	movsd
	;skip another 24 bytes
	add	edi, 24
	loop .b
	;restore VGA state to normal operation
	mov	ax, 0302h
	out	dx, ax
	mov	ax, 0204h
	out	dx, ax
	mov	dx, 03ceh
	mov	ax, 1005h
	out	dx, ax
	mov	ax, 0E06h
	out	dx, ax
	ret
	
WriteFont16px:
	mov esi, [esp + 4]
	mov edi, 0xC00A0000
	;in: edi=4k buffer
	;out: buffer filled with font
	;clear even/odd mode
	mov	dx, 03ceh
	mov	ax, 5
	out	dx, ax
	;map VGA memory to 0A0000h
	mov	ax, 0406h
	out	dx, ax
	;set bitplane 2
	mov	dx, 03c4h
	mov	ax, 0402h
	out	dx, ax
	;clear even/odd mode (the other way, don't ask why)
	mov	ax, 0604h
	out	dx, ax
	;copy charmap
	mov	ecx, 256
	;copy 16 bytes to bitmap
.b:	movsd
	movsd
    movsd
	movsd
	;skip another 16 bytes
	add	edi, 16
	loop .b
	;restore VGA state to normal operation
	mov	ax, 0302h
	out	dx, ax
	mov	ax, 0204h
	out	dx, ax
	mov	dx, 03ceh
	mov	ax, 1005h
	out	dx, ax
	mov	ax, 0E06h
	out	dx, ax
	ret

global KeIdtLoad
; requires a phys address.
KeIdtLoad:
	mov edx, [esp + 4]
	lidt [edx]
	sti
	ret

	
global g_cpuidLastLeaf
global g_cpuidNameEBX
global g_cpuidNameECX
global g_cpuidNameEDX
global g_cpuidNameNUL
global g_cpuidFeatureBits
	
global KeCPUID
KeCPUID:
	MOV EAX, 0 ; First leaf of CPUID
	
	CPUID
	
	MOV [g_cpuidLastLeaf], EAX
	MOV [g_cpuidNameEBX],  EBX
	MOV [g_cpuidNameEDX],  EDX
	MOV [g_cpuidNameECX],  ECX
	MOV DWORD [g_cpuidNameNUL], 0x0
	
	MOV EAX, 1 ; Second leaf of CPUID
	CPUID
	MOV [g_cpuidFeatureBits], EAX
	
	RET
	
global __udivdi3 ; Unsigned division of two 64-bit integers
__udivdi3:
	push    ebx
	mov     ebx, [esp+14h]
	bsr     ecx, ebx
	jz      short loc_8000087
	mov     eax, [esp+10h]
	shr     eax, cl
	shr     eax, 1
	not     ecx
	shl     ebx, cl
	or      ebx, eax
	mov     edx, [esp+0Ch]
	mov     eax, [esp+8]
	cmp     edx, ebx
	jnb     short loc_8000052
	div     ebx
	push    edi
	not     ecx
	shr     eax, 1
	shr     eax, cl
	mov     edi, eax
	mul     dword [esp+14h]
	mov     ebx, [esp+0Ch]
	mov     ecx, [esp+10h]
	sub     ebx, eax
	sbb     ecx, edx
	mov     eax, [esp+18h]
	imul    eax, edi
	sub     ecx, eax
	sbb     edi, 0
	xor     edx, edx
	mov     eax, edi
	pop     edi
	pop     ebx
	retn
; ---------------------------------------------------------------------------

loc_8000052:                            ; CODE XREF: .text:08000022↑j
	sub     edx, ebx
	div     ebx
	push    edi
	not     ecx
	shr     eax, 1
	or      eax, 80000000h
	shr     eax, cl
	mov     edi, eax
	mul     dword [esp+14h]
	mov     ebx, [esp+0Ch]
	mov     ecx, [esp+10h]
	sub     ebx, eax
	sbb     ecx, edx
	mov     eax, [esp+18h]
	imul    eax, edi
	sub     ecx, eax
	sbb     edi, 0
	xor     edx, edx
	mov     eax, edi
	pop     edi
	pop     ebx
	retn
; ---------------------------------------------------------------------------

loc_8000087:                            ; CODE XREF: .text:08000008↑j
	mov     eax, [esp+0Ch]
	mov     ecx, [esp+10h]
	xor     edx, edx
	div     ecx
	mov     ebx, eax
	mov     eax, [esp+8]
	div     ecx
	mov     edx, ebx
	pop     ebx
	retn

; It didn't used to need this, now it does!?
global __umoddi3
__umoddi3:
                push    ebx
                mov     ebx, [esp+14h]
                bsr     ecx, ebx
                jz      loc_8000196
                mov     eax, [esp+10h]
                shr     eax, cl
                shr     eax, 1
                not     ecx
                shl     ebx, cl
                or      ebx, eax
                mov     edx, [esp+0Ch]
                mov     eax, [esp+8]
                cmp     edx, ebx
                jnb     short loc_800015A
                div     ebx
                push    edi
                not     ecx
                shr     eax, 1
                shr     eax, cl
                mov     edi, eax
                mul     dword [esp+14h]
                mov     ebx, [esp+0Ch]
                mov     ecx, [esp+10h]
                sub     ebx, eax
                sbb     ecx, edx
                mov     eax, [esp+18h]
                imul    eax, edi
                sub     ecx, eax
                jnb     short loc_8000153
                add     ebx, [esp+14h]
                adc     ecx, [esp+18h]

loc_8000153:                            ; CODE XREF: .text:08000149↑j
                mov     eax, ebx
                mov     edx, ecx
                pop     edi
                pop     ebx
                retn
; ---------------------------------------------------------------------------

loc_800015A:                            ; CODE XREF: .text:08000123↑j
                sub     edx, ebx
                div     ebx
                push    edi
                not     ecx
                shr     eax, 1
                or      eax, 80000000h
                shr     eax, cl
                mov     edi, eax
                mul     dword [esp+14h]
                mov     ebx, [esp+0Ch]
                mov     ecx, [esp+10h]
                sub     ebx, eax
                sbb     ecx, edx
                mov     eax, [esp+18h]
                imul    eax, edi
                sub     ecx, eax
                jnb     short loc_800018F
                add     ebx, [esp+14h]
                adc     ecx, [esp+18h]

loc_800018F:                            ; CODE XREF: .text:08000185↑j
                mov     eax, ebx
                mov     edx, ecx
                pop     edi
                pop     ebx
                retn
; ---------------------------------------------------------------------------

loc_8000196:                            ; CODE XREF: .text:08000105↑j
                mov     eax, [esp+0Ch]
                mov     ecx, [esp+10h]
                xor     edx, edx
                div     ecx
                mov     ebx, eax
                mov     eax, [esp+8]
                div     ecx
                mov     eax, edx
                pop     ebx
                xor     edx, edx
                retn

extern _kernel_end
global MmStartupStuff
MmStartupStuff:
; WORK: Change this if necessary.  Paging is not setup at this stage
;       so this address is purely PHYSICAL.

; TODO: Compilation seems to include stuff like CODE_SEG, DATA_SEG, VMWARE_MAGIC
;       etc after our bss, which may not work.
; Use this weird math to move the placement, in order to be page aligned, as the
; system loves it that way.

	mov ecx, (_kernel_end - 0xC0000000 + 0x1000)
	
	; couldn't do the and itself during compilation :(
	and ecx, 0xFFFFF000
	mov dword [e_placement], ecx
	ret
	
	
section .bss

; eax=0, eax's value:
g_cpuidLastLeaf resd 1

; eax=0, the processor name (GenuineIntel, AuthenticAMD etc):
g_cpuidNameEBX resd 1
g_cpuidNameEDX resd 1
g_cpuidNameECX resd 1
g_cpuidNameNUL resd 1

; eax=1, eax's value:
g_cpuidFeatureBits resd 1