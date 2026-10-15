#
# Chinese translation for luci-app-nikki
#
# Prebuilt package: the payload is the exact file set the device already runs,
# so a fresh flash carries it without any manual apk work.  No compilation.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=luci-i18n-nikki-zh-cn
PKG_VERSION:=26.161.56550~95d41dd
PKG_RELEASE:=1

PKG_MAINTAINER:=OpenWrt MT6990 LG6851F integration
PKG_LICENSE:=Apache-2.0
PKGARCH:=all

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/luci-i18n-nikki-zh-cn
  SECTION:=luci
  CATEGORY:=LuCI
  TITLE:=Chinese translation for luci-app-nikki
  DEPENDS:=+luci-app-nikki
  PROVIDES:=
endef

define Package/luci-i18n-nikki-zh-cn/description
Chinese translation for luci-app-nikki
endef

define Build/Compile
endef

define Package/luci-i18n-nikki-zh-cn/install
	$(CP) ./files/. $(1)/
endef

$(eval $(call BuildPackage,luci-i18n-nikki-zh-cn))
