AS=$(CROSS_COMPILE)as
LD=$(CROSS_COMPILE)ld
OBJCOPY=$(CROSS_COMPILE)objcopy

all: n64linux.n64

n64load.o: n64load.s vmlinux.bin
	$(AS) -o $@ $<

# note: abusing $+ as first(ldscript) and rest(objects).
n64linux.elf: n64linux.lds n64load.o
	$(LD) -o $@ -T $+

n64linux.bin: n64linux.elf
	$(OBJCOPY) -O binary -j .text -j .data $< $@

# FIXME: must adjust -l arg by n64linux.bin size.
n64linux.n64: n64linux.bin
	$(N64TOOL) -l 4M -h $(N64HEADER) -o $@ -t "N64Linux" $<
	echo -e '\x80\x00\x10\x00' | dd of=$@ bs=1 count=4 seek=8 conv=notrunc
	$(N64CHKSUM) $@
