AS=$(CROSS_COMPILE)as
LD=$(CROSS_COMPILE)ld
OBJCOPY=$(CROSS_COMPILE)objcopy
ASFLAGS += -march=vr4300

all: n64load.bin mkn64bin

n64load.o: n64load.s
	$(AS) $(ASFLAGS) -o $@ $<

# note: libdragon's "header" sets entry at 0x80000400.
n64load.elf: n64load.o
	$(LD) -Ttext 0x80000400 -o $@ $<

n64load.bin: n64load.elf
	$(OBJCOPY) -O binary -j .text $< $@

# FIXME: must adjust -l arg by n64linux.bin size.
#n64linux.n64: n64linux.bin
#	$(N64TOOL) -l 4M -h $(N64HEADER) -o $@ -t "N64Linux" $<
#	$(N64CHKSUM) $@

# host tools
mkn64bin: mkn64bin.c
	$(HOSTCC) -o $@ $<
