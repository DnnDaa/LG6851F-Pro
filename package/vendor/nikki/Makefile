#
# Nikki transparent proxy with Mihomo
#
# Prebuilt package: the payload is the exact file set the device already runs,
# so a fresh flash carries it without any manual apk work.  No compilation.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=nikki
PKG_VERSION:=2026.04.08
PKG_RELEASE:=1

PKG_MAINTAINER:=OpenWrt MT6990 LG6851F integration
PKG_LICENSE:=GPL3.0+
PKGARCH:=aarch64_generic

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/nikki
  SECTION:=net
  CATEGORY:=Network
  TITLE:=Nikki transparent proxy with Mihomo
  DEPENDS:=+mihomo +yq +curl +ca-bundle +firewall4 +ip-full +kmod-dummy +kmod-inet-diag +kmod-nft-socket +kmod-nft-tproxy +kmod-tun
  PROVIDES:=
endef

define Package/nikki/description
Nikki transparent proxy with Mihomo
endef

define Build/Compile
endef

define Package/nikki/install
	$(CP) ./files/. $(1)/
endef

$(eval $(call BuildPackage,nikki))
