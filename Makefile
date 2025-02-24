CC = gcc
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -Wall -Wextra -Wno-override-init
ASM = nasm
ASMFLAGS = -f elf32

OBJS = boot.o kernel.o

all: os.iso

os.iso: kernel.bin
	cp kernel.bin os.iso

kernel.bin: $(OBJS)
	ld -m elf_i386 -T linker.ld -o kernel.bin $(OBJS)

boot.o: boot.asm
	$(ASM) $(ASMFLAGS) boot.asm -o boot.o

kernel.o: kernel.c
	$(CC) $(CFLAGS) -c kernel.c -o kernel.o

clean:
	rm -f *.o *.bin *.iso

run: os.iso
	qemu-system-i386 -kernel kernel.bin
