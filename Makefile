CC = gcc
LD = ld
ASM = nasm
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs -ffreestanding -I.
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

all: ouros.iso

boot.o: boot.asm
	$(ASM) $(ASFLAGS) boot.asm -o boot.o

kernel.o: kernel.c
	$(CC) $(CFLAGS) -c kernel.c -o kernel.o

ouros.bin: boot.o kernel.o
	$(LD) $(LDFLAGS) -o ouros.bin boot.o kernel.o

ouros.iso: ouros.bin
	mkdir -p iso/boot/grub
	cp ouros.bin iso/boot/
	echo 'set timeout=0' > iso/boot/grub/grub.cfg
	echo 'set default=0' >> iso/boot/grub/grub.cfg
	echo 'menuentry "OUROS OS" {' >> iso/boot/grub/grub.cfg
	echo '  multiboot /boot/ouros.bin' >> iso/boot/grub/grub.cfg
	echo '  boot' >> iso/boot/grub/grub.cfg
	echo '}' >> iso/boot/grub/grub.cfg
	grub-mkrescue -o ouros.iso iso

run: ouros.iso
	qemu-system-i386 -cdrom ouros.iso -m 64M

run-serial: ouros.iso
	qemu-system-i386 -cdrom ouros.iso -m 64M -display none -serial stdio

clean:
	rm -rf *.o *.bin *.iso iso/

install-deps:
	sudo apt update
	sudo apt install -y nasm make gcc-multilib grub-common grub-pc-bin xorriso qemu-system-x86

.PHONY: all clean run run-serial install-deps
