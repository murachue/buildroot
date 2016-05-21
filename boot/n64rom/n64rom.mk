################################################################################
#
# n64rom
#
################################################################################

N64ROM_VERSION = 1
N64ROM_LICENSE = Proprietary

N64ROM_SITE_METHOD = local
N64ROM_SITE = $(TOPDIR)/boot/n64rom/src

N64ROM_DEPENDENCIES = host-libdragon linux

N64ROM_MAKE_OPTS = \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	N64TOOL="$(HOST_DIR)/usr/bin/n64tool" \
	N64CHKSUM="$(HOST_DIR)/usr/bin/chksum64" \
	N64HEADER="$(HOST_DIR)/usr/lib/header" \

# I use LINUX_IMAGE_NAME, but it should be vmlinux.bin.
define N64ROM_BUILD_CMDS
	cp $(BINARIES_DIR)/$(LINUX_IMAGE_NAME) $(@D)/vmlinux.bin
	$(TARGET_MAKE_ENVS) $(MAKE) $(N64ROM_MAKE_OPTS) -C $(@D)
endef

define N64ROM_INSTALL_TARGET_CMDS
	#cp $(@D)/n64linux.n64 $(BINARIES_DIR)/
endef

$(eval $(generic-package))
