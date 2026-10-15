ifneq ($(findstring mt7915,$(CHIPSET)),)
CHIP_7915_PATH := $(PARENT_DIR)/chip/mt7915

obj_chips += \
	$(CHIP_7915_PATH)/mt7915.o\
	$(CHIP_7915_PATH)/mt7915_dbg.o
endif
