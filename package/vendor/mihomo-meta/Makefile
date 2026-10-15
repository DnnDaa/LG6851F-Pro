#
# Mihomo Meta core (prebuilt, provides mihomo)
#
# Prebuilt package: the payload is the exact file set the device already runs,
# so a fresh flash carries it without any manual apk work.  No compilation.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=mihomo-meta
PKG_VERSION:=1.19.27
PKG_RELEASE:=1

PKG_MAINTAINER:=OpenWrt MT6990 LG6851F integration
PKG_LICENSE:=GPL-3.0-only
PKGARCH:=aarch64_generic

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/mihomo-meta
  SECTION:=net
  CATEGORY:=Network
  TITLE:=Mihomo Meta core (prebuilt, provides mihomo)
  DEPENDS:=+libc +kmod-inet-diag +kmod-tun +ip-full +ca-bundle
  PROVIDES:=mihomo
endef

define Package/mihomo-meta/description
Mihomo Meta core (prebuilt, provides mihomo)
endef

define Build/Compile
endef

define Package/mihomo-meta/install
	$(CP) ./files/. $(1)/
endef

$(eval $(call BuildPackage,mihomo-meta))
