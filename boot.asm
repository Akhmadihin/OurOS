bits 32

; Multiboot header (должен быть в первых 8KB файла)
section .multiboot
align 4
multiboot_header:
    dd 0x1BADB002              ; magic number
    dd 0x00000003              ; flags
    dd -(0x1BADB002 + 0x00000003) ; checksum

; Stack section
section .bss
align 16
stack_bottom:
    resb 16384                 ; 16KB stack
stack_top:

; Kernel entry point
section .text
global _start
extern kernel_main

_start:
    ; Set up stack
    mov esp, stack_top
    
    ; Push Multiboot info (GRUB передает в регистрах)
    push ebx                   ; Multiboot info structure
    push eax                   ; Multiboot magic number (0x2BADB002)
    
    ; Call kernel main
    call kernel_main
    
    ; Halt if kernel returns
    cli
    hlt

; Simple print function for early debugging
global early_print
early_print:
    pusha
    mov edi, 0xB8000
.early_print_loop:
    mov al, [esi]
    test al, al
    jz .early_print_done
    mov [edi], al
    add edi, 2
    inc esi
    jmp .early_print_loop
.early_print_done:
    popa
    ret
