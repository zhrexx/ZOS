CC = gcc
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -Wall -Wextra -Wno-override-init -static -ffreestanding
ASM = nasm
ASMFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

ISO_DIR = isodir
BOOT_DIR = $(ISO_DIR)/boot
GRUB_DIR = $(ISO_DIR)/boot/grub
OBJS = boot.o kernel.o 

all: os.iso

os.iso: kernel.elf
	mkdir -p $(GRUB_DIR)
	cp kernel.elf $(BOOT_DIR)
	echo 'menuentry "ZOS" {' > $(GRUB_DIR)/grub.cfg
	echo '  multiboot /boot/kernel.elf' >> $(GRUB_DIR)/grub.cfg
	echo '  boot' >> $(GRUB_DIR)/grub.cfg
	echo '}' >> $(GRUB_DIR)/grub.cfg
	grub2-mkrescue -o os.iso $(ISO_DIR)

kernel.elf: $(OBJS)
	ld $(LDFLAGS) -o $@ $^

%.o: %.asm
	$(ASM) $(ASMFLAGS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf *.o *.elf *.iso $(ISO_DIR)

run: os.iso disk.img
	qemu-system-i386 -cdrom os.iso -drive file=disk.img,format=raw -boot d -serial stdio -vga std

disk.img: 
	./disk_util create disk.img 64
	./disk_util format disk.img

.PHONY: all clean run
