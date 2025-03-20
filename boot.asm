section .multiboot
align 4
    dd 0x1BADB002
    dd 0x07
    dd -(0x1BADB002 + 0x07)
    dd 0
    dd 0
    dd 0
    dd 0
    dd 0
    dd 0
    dd 1024
    dd 768
    dd 32

section .text
global _start
extern kernel_main 
_start:
    cli
    mov esp, stack_top 
    push ebx
    push eax 

    call kernel_main
    cli
.hang:
    hlt
    jmp .hang

section .bss
regs: resb 32
align 16
stack_bottom:
    resb 16384
stack_top:

