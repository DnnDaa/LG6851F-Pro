#
# LuCI support for Nikki
#
# Prebuilt package: the payload is the exact file set the device already runs,
# so a fresh flash carries it without any manual apk work.  No compilation.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=luci-app-nikki
PKG_VERSION:=1.26.1
PKG_RELEASE:=1

PKG_MAINTAINER:=OpenWrt MT6990 LG6851F integration
PKG_LICENSE:=Apache-2.0
PKGARCH:=all

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/luci-app-nikki
  SECTION:=luci
  CATEGORY:=LuCI
  TITLE:=LuCI support for Nikki
  DEPENDS:=+luci-base +rpcd-mod-ucode +nikki
  PROVIDES:=
endef

define Package/luci-app-nikki/description
LuCI support for Nikki
endef

define Build/Compile
endef

define Package/luci-app-nikki/install
	$(CP) ./files/. $(1)/
endef

$(eval $(call BuildPackage,luci-app-nikki))
