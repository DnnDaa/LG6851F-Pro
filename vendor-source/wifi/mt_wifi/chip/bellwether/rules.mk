ifneq ($(findstring bellwether,$(CHIPSET)),)
CHIP_BELLWETHER_PATH := $(PARENT_DIR)/chip/bellwether

obj_chips += \
	$(CHIP_BELLWETHER_PATH)/bellwether.o\
	$(CHIP_BELLWETHER_PATH)/bellwether_dbg.o\
	$(CHIP_BELLWETHER_PATH)/bellwether_ser.o
endif
