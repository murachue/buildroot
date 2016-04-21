################################################################################
#
# libdragon
#
################################################################################

LIBDRAGON_VERSION = $(call qstrip,$(BR2_LIBDRAGON_VERSION))
LIBDRAGON_LICENSE = Public domain
LIBDRAGON_LICENSE_FILES = LICENSE.md

# TODO this ifeq-cascade should be made more good.
ifeq ($(BR2_LIBDRAGON_STABLE_VERSION),y)
#LIBDRAGON_SITE = $(call github,DragonMinded,libdragon,$(LIBDRAGON_VERSION))
LIBDRAGON_SITE = https://dragonminded.com/n64dev/
LIBDRAGON_SOURCE = libdragon-$(LIBDRAGON_VERSION).tgz
else ifeq ($(BR2_LIBDRAGON_LATEST_VERSION),y)
LIBDRAGON_SITE_METHOD = git
LIBDRAGON_SITE = https://github.com/DragonMinded/libdragon.git
else ifeq ($(BR2_LIBDRAGON_CUSTOM_LOCAL),y)
LIBDRAGON_SITE_METHOD = local
LIBDRAGON_SITE = $(call qstrip,$(BR2_LIBDRAGON_CUSTOM_LOCAL_PATH))
else
$(error unknown site for libdragon)
endif

LIBDRAGON_INSTALL_STAGING = YES
LIBDRAGON_INSTALL_TARGET = NO
# below line does not effect...?
#HOST_LIBDRAGON_INSTALL_STAGING = YES

# make build n64tools only, if we don't want to build libdragon tools.
ifneq ($(BR2_PACKAGE_HOST_LIBDRAGON_ALLTOOLS),y)
# Prevent libdragon to build the libdragon-related tools, that is not required.
define LIBDRAGON_DISABLE_LDTOOLS
	$(SED) '/^build:/ s/dumpdfs mkdfs mksprite//' $(@D)/tools/Makefile
	$(SED) '/^install:/ s/dumpdfs-install mkdfs-install mksprite-install//' $(@D)/tools/Makefile
endef
#LIBDRAGON_PRE_BUILD_HOOKS += LIBDRAGON_DISABLE_LDTOOLS
HOST_LIBDRAGON_PRE_BUILD_HOOKS += LIBDRAGON_DISABLE_LDTOOLS
endif

define LIBDRAGON_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) N64_INST="$(STAGING_DIR)" N64PREFIX="$(TARGET_CROSS)" -C $(@D)
endef
define LIBDRAGON_INSTALL_STAGING_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) N64_INST="$(STAGING_DIR)" N64PREFIX="$(TARGET_CROSS)" -C $(@D) install
endef

define HOST_LIBDRAGON_BUILD_CMDS
	$(HOST_MAKE_ENV) $(MAKE) N64_INST="$(HOST_DIR)/usr" -C $(@D)/tools
endef
define HOST_LIBDRAGON_INSTALL_CMDS
	$(HOST_MAKE_ENV) $(MAKE) N64_INST="$(HOST_DIR)/usr" -C $(@D)/tools install
	install -D --mode=644 $(@D)/header $(HOST_DIR)/usr/lib/header
endef

# libdragon for Linux userland is not supported...
#$(eval $(generic-package))
$(eval $(host-generic-package))
