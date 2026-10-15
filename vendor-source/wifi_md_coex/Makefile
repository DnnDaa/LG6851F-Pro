ifneq ($(CONFIG_SUPPORT_OPENWRT),)

obj-$(CONFIG_COEXISTENCE_SUPPORT) := wifi_md_coex.o
wifi_md_coex-objs = mt_wifi_md_coex.o fw2apccci_ctrl_cmd.o

else

PWD = $(shell pwd)
KERNEL_DIR = $(PWD)/../../$(LINUXDIR)
ROMFS_DIR = $(PWD)/../../romfs
#@echo $(PWD);
#ccflags-y := -I../proprietary_driver/drivers/cryptodev

$(warning test =$(PWD))
ccflags-y = -Wall -Wstrict-prototypes -Wno-trigraphs -Wframe-larger-than=4096 -Werror
ccflags-y += -I$(PWD)/drivers/misc/mediatek/include
ccflags-y += -I$(PWD)/drivers/misc/mediatek/include/mt-plat

obj-m := wifi_md_coex.o
wifi_md_coex-objs = mt_wifi_md_coex.o fw2apccci_ctrl_cmd.o

modules:
	make -C $(KERNEL_DIR) M=$(PWD) modules

modules_install:
	make INSTALL_MOD_PATH=$(ROMFS_DIR) -C $(KERNEL_DIR) M=$(PWD) modules_install

clean:
	make -C $(KERNEL_DIR) M=$(PWD) clean

endif