PIC1_COMMAND    equ 0x20
PIC1_DATA       equ 0x21
PIC2_COMMAND    equ 0xA0
PIC2_DATA       equ 0xA1

section .multiboot
align 4
    dd 0x1BADB002
    dd 0x00
    dd -(0x1BADB002 + 0x00)

section .text
global _start
global init_pic
global init_idt
global keyboard_handler
extern kernel_main

keyboard_handler:
    pusha
    popa
    iret

init_pic:
    push ebp
    mov ebp, esp

    mov al, 0x11
    out PIC1_COMMAND, al
    out PIC2_COMMAND, al

    mov al, 0x20
    out PIC1_DATA, al
    mov al, 0x28
    out PIC2_DATA, al

    mov al, 0x04
    out PIC1_DATA, al
    mov al, 0x02
    out PIC2_DATA, al

    mov al, 0x01
    out PIC1_DATA, al
    out PIC2_DATA, al

    mov al, 0xFF
    out PIC1_DATA, al
    out PIC2_DATA, al

    mov esp, ebp
    pop ebp
    ret

init_idt:
    push ebp
    mov ebp, esp

    mov ecx, 256*8
    mov edi, idt_table
    xor eax, eax
    rep stosb

    mov edi, idt_table + (33 * 8)
    mov eax, keyboard_handler
    mov word [edi], ax
    mov word [edi + 2], 0x08
    mov byte [edi + 4], 0
    mov byte [edi + 5], 0x8E
    shr eax, 16
    mov word [edi + 6], ax

    lidt [idt_pointer]

    mov esp, ebp
    pop ebp
    ret

_start:
    cli
    mov esp, stack_top
    push ebx
    push eax
    call init_pic
    call init_idt
    sti
    call kernel_main
    hlt

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

section .data 
idt_table:      times 256*8 db 0
idt_pointer:    
    dw (256*8)-1
    dd idt_table
