#
# luci-app-fiberhomedash - LG6851F / MT6990 status dashboard (fiberhome style)
#
# SIP: script-only package (ucode backend + LuCI JS view + i18n), so PKGARCH is
# all/noarch.  Built inside the OpenWrt tree so the .apk is signed with the
# same key the firmware already trusts (/etc/apk/keys/public-key.pem).
#
include $(TOPDIR)/rules.mk

PKG_NAME:=luci-app-fiberhomedash
PKG_VERSION:=1.0.0
PKG_RELEASE:=1

PKG_MAINTAINER:=OpenWrt MT6990 LG6851F integration
PKG_LICENSE:=Apache-2.0

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/luci-app-fiberhomedash
  SECTION:=luci
  CATEGORY:=LuCI
  SUBMENU:=3. Applications
  TITLE:=LG6851F / MT6990 status dashboard (fiberhome style)
  DEPENDS:=+luci-base +rpcd-mod-ucode +ucode +ucode-mod-fs +ucode-mod-math
  PKGARCH:=all
endef

define Package/luci-app-fiberhomedash/description
  FiberHome LG6851F Pro (MT6990) live status dashboard.  CPU / memory / storage /
  temperature, cellular signal, carrier aggregation, QCI / AMBR, DL-UL MCS, module
  identity and Wi-Fi clients.  One low-priority procd sampler owns every periodic
  read, so the LuCI page and the rpcd backend stay cheap and the sampler idles by
  itself when nobody is looking.
endef

define Build/Compile
endef

define Package/luci-app-fiberhomedash/install
	$(INSTALL_DIR) $(1)/usr/share/rpcd/ucode
	$(INSTALL_BIN) ./files/usr/share/rpcd/ucode/fiberhomedash $(1)/usr/share/rpcd/ucode/fiberhomedash
	$(INSTALL_DIR) $(1)/www/luci-static/resources/view/fiberhome
	$(INSTALL_DATA) ./files/www/luci-static/resources/view/fiberhome/home.js $(1)/www/luci-static/resources/view/fiberhome/home.js
	$(INSTALL_DIR) $(1)/www/luci-static/resources/fiberhome
	$(INSTALL_DATA) ./files/www/luci-static/resources/fiberhome/common.js $(1)/www/luci-static/resources/fiberhome/common.js
	$(INSTALL_DIR) $(1)/usr/share/luci/menu.d
	$(INSTALL_DATA) ./files/usr/share/luci/menu.d/luci-app-fiberhome.json $(1)/usr/share/luci/menu.d/luci-app-fiberhome.json
	$(INSTALL_DIR) $(1)/usr/share/rpcd/acl.d
	$(INSTALL_DATA) ./files/usr/share/rpcd/acl.d/luci-app-fiberhome.json $(1)/usr/share/rpcd/acl.d/luci-app-fiberhome.json
	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/i18n
	$(INSTALL_DATA) ./files/usr/lib/lua/luci/i18n/fiberhome.zh-cn.lmo $(1)/usr/lib/lua/luci/i18n/fiberhome.zh-cn.lmo
	$(INSTALL_DATA) ./files/usr/lib/lua/luci/i18n/fiberhomedash.zh-cn.lmo $(1)/usr/lib/lua/luci/i18n/fiberhomedash.zh-cn.lmo
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/fiberhomedash $(1)/etc/init.d/fiberhomedash
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) ./files/etc/config/fiberhomedash $(1)/etc/config/fiberhomedash
endef

define Package/luci-app-fiberhomedash/postinst
#!/bin/sh
[ "$${IPKG_NO_SCRIPT}" = "1" ] && exit 0
[ -s "$${IPKG_INSTROOT}/lib/functions.sh" ] || exit 0
. "$${IPKG_INSTROOT}/lib/functions.sh"
default_postinst "$$0" "$$@"
[ -n "$${IPKG_INSTROOT}" ] || {
	/etc/init.d/fiberhomedash enable >/dev/null 2>&1
	/etc/init.d/fiberhomedash restart >/dev/null 2>&1
	/etc/init.d/rpcd restart >/dev/null 2>&1
}
exit 0
endef

define Package/luci-app-fiberhomedash/prerm
#!/bin/sh
[ -n "$${IPKG_INSTROOT}" ] || {
	/etc/init.d/fiberhomedash stop >/dev/null 2>&1
	/etc/init.d/fiberhomedash disable >/dev/null 2>&1
}
exit 0
endef

$(eval $(call BuildPackage,luci-app-fiberhomedash))
