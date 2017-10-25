ifneq ($(filter am57xevm%, $(TARGET_DEVICE)),)

ifeq ($(TARGET_BUILD_KERNEL), true)

$(PRODUCT_OUT)/am57xx-evm-reva3.dtb: all_dtbs
	cp -uvf $(PRODUCT_OUT)/obj/kernel/arch/arm/boot/am57xx-evm-reva3.dtb $(PRODUCT_OUT)/am57xx-evm-reva3.dtb

$(INSTALLED_BOOTIMAGE_TARGET): ${PRODUCT_OUT}/am57xx-evm-reva3.dtb

$(KERNELDIR)/.version: $(INSTALLED_KERNEL_TARGET)

$(BOARD_VENDOR_KERNEL_MODULES): $(INSTALLED_KERNEL_TARGET)
$(BOARD_RECOVERY_KERNEL_MODULES): $(INSTALLED_KERNEL_TARGET)

endif
endif
