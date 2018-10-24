all: $(BINARIES_DIR)/n64linux.n64

$(BINARIES_DIR)/n64linux.n64: $(BINARIES_DIR)/n64linux.bin
	@$(call MESSAGE,"Finalizing the master file to the ROM file")
	$(HOST_DIR)/usr/bin/n64tool -l 4M -h $(HOST_DIR)/usr/lib/header -o $(BINARIES_DIR)/n64linux.n64 -t "N64Linux" $(BINARIES_DIR)/n64linux.bin
	$(HOST_DIR)/usr/bin/chksum64 $(BINARIES_DIR)/n64linux.n64

# TODO this requires linux and n64rom, add to deps, or user's responsibility (nconfig)?
# note: MESSAGE macro's arg can not contain comma
ifneq ($(filter rootfs-iso9660,$(TARGETS_ROOTFS)),)
$(BINARIES_DIR)/n64linux.bin: world
	@$(call MESSAGE,"Packing boot-loader/kernel/rootfs into the master file")
	$(HOST_DIR)/usr/bin/mkn64bin.iso9660 $(BINARIES_DIR)/n64linux.bin $(BINARIES_DIR)/n64load.bin "BOOT/VMLINUX.BIN;1" $(BINARIES_DIR)/rootfs.$(patsubst rootfs-%,%,$(filter rootfs-iso9660,$(TARGETS_ROOTFS)))
else
$(BINARIES_DIR)/n64linux.bin: world
	@$(call MESSAGE,"Packing boot-loader/kernel/rootfs into the master file")
	$(HOST_DIR)/usr/bin/mkn64bin $(BINARIES_DIR)/n64linux.bin $(BINARIES_DIR)/n64load.bin $(BINARIES_DIR)/vmlinux.bin $(BINARIES_DIR)/rootfs.$(patsubst rootfs-%,%,$(TARGETS_ROOTFS))
endif
