OUTPUT_FORMAT(elf32-i386)
ENTRY(_start)

SECTIONS {
    . = 1M;
    .text BLOCK(4K) : ALIGN(4K) {
        *(.multiboot)
        *(.text)
    }
    
    .rodata BLOCK(4K) : ALIGN(4K) {
        *(.rodata*)
    }
    
    .data BLOCK(4K) : ALIGN(4K) {
        *(.data)
    }
    
    .bss BLOCK(4K) : ALIGN(4K) {
        *(COMMON)
        *(.bss)
        . = ALIGN(16);
        stack_bottom = .;
        . += 16K;
        stack_top = .;
    }
}
