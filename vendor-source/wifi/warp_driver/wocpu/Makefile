ccflags-y += -I$(src)/include -Inet/nat -I$(src)/../warp -I$(src)/../warp/mcu -I$(src)/driver/hal/inc -I$(src)/coda_gen/hw_cr -I$(src)/coda_gen/sw_define

ifeq ($(CONFIG_WOCPU_HOST), y)
	ccflags-y += -DCONFIG_WOCPU_HOST -DCONFIG_WED_HW_RRO_SUPPORT
endif

MODULE_NAME=mtk_wo

OBJS:= system/task/wo_ko.o
OBJS += system/task/wo_mem.o
OBJS += system/qm/wo_miod.o
OBJS += system/qm/wo_fbcmd.o
OBJS += system/qm/wo_qm.o
OBJS += system/rxm/rro_array.o
OBJS += system/rxm/rro_list.o
OBJS += system/rxm/rxm.o
OBJS += system/rxm/dev.o
OBJS += system/rxm/bss.o
OBJS += system/rxm/sta_info.o
OBJS += system/rxm/fmac.o
OBJS += system/rxm/wpa.o
OBJS += system/cmd/wo_fwcmd.o
OBJS += system/cmd/wo_dbg.o
OBJS += driver/hal/hal_wed.o

#######################################
# For integrating to kernel source
######################################
obj-m += $(MODULE_NAME).o


$(MODULE_NAME)-objs += $(OBJS)

install:

clean:
	rm -rf *.o *.ko *.modules .*.cmd .*.o *.order $(MODULE_NAME).mod.c
