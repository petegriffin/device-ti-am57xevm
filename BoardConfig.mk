#
# Copyright (C) 2011 The Android Open-Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# These two variables are set first, so they can be overridden
# by BoardConfigVendor.mk
#BOARD_USES_GENERIC_AUDIO := true
#USE_CAMERA_STUB := true
OMAP_ENHANCEMENT := true

ifeq ($(OMAP_ENHANCEMENT),true)
#COMMON_GLOBAL_CFLAGS += -DOMAP_ENHANCEMENT
endif

TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_CPU_SMP := true
TARGET_ARCH := arm
TARGET_ARCH_VARIANT := armv7-a-neon
TARGET_CPU_VARIANT := cortex-a15

ENABLE_CPUSETS := true

#BOARD_HAVE_BLUETOOTH := false
#BOARD_HAVE_BLUETOOTH_TI := false
#BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := device/ti/am57xevm/bluetooth
TARGET_NO_BOOTLOADER := true

BOARD_KERNEL_BASE := 0x80000000
#BOARD_KERNEL_CMDLINE := console=ttyO2,115200n8 mem=1024M androidboot.console=ttyO2 androidboot.hardware=am57xevmboard vram=20M omapfb.vram=0:16M
BOARD_MKBOOTIMG_ARGS := --ramdisk_offset 0x03000000

TARGET_NO_RADIOIMAGE := true
TARGET_BOARD_PLATFORM := am57x
TARGET_BOOTLOADER_BOARD_NAME := am57xevm

BOARD_EGL_CFG := device/ti/am57xevm/egl.cfg

USE_OPENGL_RENDERER := true

TARGET_USERIMAGES_USE_EXT4 := true
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 805306368
BOARD_USERDATAIMAGE_PARTITION_SIZE := 2147483648
BOARD_CACHEIMAGE_PARTITION_SIZE := 268435456
BOARD_CACHEIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_FLASH_BLOCK_SIZE := 4096

BOARD_VENDORIMAGE_PARTITION_SIZE := 268435456
BOARD_VENDORIMAGE_FILE_SYSTEM_TYPE := ext4
TARGET_COPY_OUT_VENDOR := vendor

TARGET_RECOVERY_FSTAB = device/ti/am57xevm/fstab.am57xevmboard
TARGET_RECOVERY_PIXEL_FORMAT := "BGRA_8888"
TARGET_RELEASETOOLS_EXTENSIONS := device/ti/am57xevm

# Connectivity - Wi-Fi
USES_TI_MAC80211 := false
ifeq ($(USES_TI_MAC80211),true)
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
WPA_SUPPLICANT_VERSION      := VER_0_8_X
BOARD_HOSTAPD_DRIVER        := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB  := lib_driver_cmd_wl12xx
BOARD_HOSTAPD_PRIVATE_LIB         := lib_driver_cmd_wl12xx
BOARD_WLAN_DEVICE           := wl12xx_mac80211
BOARD_SOFTAP_DEVICE         := wl12xx_mac80211
#COMMON_GLOBAL_CFLAGS += -DUSES_TI_MAC80211
#COMMON_GLOBAL_CFLAGS += -DANDROID_LIB_STUB
endif

BOARD_SEPOLICY_DIRS += \
	 device/ti/am57xevm/sepolicy \

# lidbrm driver
BOARD_GPU_DRIVERS := omapdrm

# DispSync vsync offsets in nanoseconds
VSYNC_EVENT_PHASE_OFFSET_NS := 7500000
SF_VSYNC_EVENT_PHASE_OFFSET_NS := 5000000

BOARD_VENDOR_KERNEL_MODULES := \
	$(KERNELDIR)/drivers/scsi/scsi_mod.ko\
	$(KERNELDIR)/drivers/scsi/sd_mod.ko\
	$(KERNELDIR)/drivers/usb/storage/usb-storage.ko
#	$(KERNELDIR)/drivers/net/wireless/ti/wl18xx/wl18xx.ko \
#	$(KERNELDIR)/drivers/net/wireless/ti/wlcore/wlcore.ko \
#	$(KERNELDIR)/drivers/net/wireless/ti/wlcore/wlcore_sdio.ko
