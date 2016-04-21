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

# only build tools for host.
HOST_LIBDRAGON_SUBDIR = tools

# We only want n64tools and not the libdragon?
ifeq ($(BR2_PACKAGE_LIBDRAGON_N64TOOLSONLY),y)
# Prevent libdragon to build the libdragon-related tools, that is not required.
define LIBDRAGON_DISABLE_LDTOOLS
	$(SED) '/^build:/ s/dumpdfs mkdfs mksprite//' $(@D)/tools/Makefile
	$(SED) '/^install:/ s/dumpdfs-install mkdfs-install mksprite-install//' $(@D)/tools/Makefile
endef
LIBDRAGON_PRE_BUILD_HOOKS += LIBDRAGON_DISABLE_LDTOOLS
HOST_LIBDRAGON_PRE_BUILD_HOOKS += LIBDRAGON_DISABLE_LDTOOLS
endif

LIBDRAGON_MAKE_ENV = \
	N64_INST="$(STAGING_DIR)"
	N64PREFIX="$(CROSS_COMPILE)"
HOST_LIBDRAGON_MAKE_ENV = \
	N64_INST="$(HOST_DIR)/usr"

# use (host-)autotools-package, but no configure, nor install to target.
# just for run make.
# set it ":" for just define but do nothing.
LIBDRAGON_CONFIGURE_CMDS = :
HOST_LIBDRAGON_CONFIGURE_CMDS = :
LIBDRAGON_INSTALL_TARGET_CMDS = :

# no install libdragon for host. only for staging.
LIBDRAGON_INSTALL_CMDS = :

$(eval $(autotools-package))
$(eval $(host-autotools-package))
