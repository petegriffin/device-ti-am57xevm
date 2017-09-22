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

ifeq ($(TARGET_PREBUILT_KERNEL),)
LOCAL_KERNEL := $(KERNELDIR)/arch/arm/boot/zImage
else
LOCAL_KERNEL := $(TARGET_PREBUILT_KERNEL)
endif

PRODUCT_PACKAGES += \
   android.hardware.wifi@1.0-service \
   android.hardware.graphics.allocator@2.0-impl \
   android.hardware.graphics.allocator@2.0-service \
   android.hardware.graphics.mapper@2.0-impl \
   android.hardware.memtrack@1.0-impl \
   android.hardware.usb@1.0-service \
   android.hardware.power@1.0-impl \
   android.hardware.audio@2.0-impl \
   android.hardware.audio@2.0-service \
   android.hardware.audio.effect@2.0-impl \
   android.hardware.broadcastradio@1.0-impl \
   android.hardware.soundtrigger@2.0-impl \
   android.hardware.keymaster@3.0-impl \
   android.hardware.keymaster@3.0-service

PRODUCT_COPY_FILES := \
	$(LOCAL_KERNEL):kernel \
	device/ti/am57xevm/tablet_core_hardware_am57xevm.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/tablet_core_hardware_am57xevm.xml \
	device/ti/am57xevm/manifest.xml:$(TARGET_COPY_OUT_VENDOR)/manifest.xml \
	device/ti/am57xevm/init.am57xevmboard.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.am57xevmboard.rc \
	device/ti/am57xevm/init.am57xevmboard.usb.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.am57xevmboard.usb.rc \
	device/ti/am57xevm/ueventd.am57xevmboard.rc:$(TARGET_COPY_OUT_VENDOR)/ueventd.rc \
	device/ti/am57xevm/fstab.am57xevmboard:$(TARGET_COPY_OUT_VENDOR)/etc/fstab.am57xevmboard \
	device/ti/am57xevm/media_profiles_V1_0.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_profiles_V1_0.xml \
	device/ti/am57xevm/media_codecs.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_codecs.xml \
	device/ti/jacinto6evm/media_codecs_performance.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_codecs_performance.xml \
	frameworks/av/media/libstagefright/data/media_codecs_google_audio.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_codecs_google_audio.xml \
	frameworks/av/media/libstagefright/data/media_codecs_google_video.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_codecs_google_video.xml \
	frameworks/native/data/etc/android.hardware.wifi.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.xml \
	frameworks/native/data/etc/android.hardware.wifi.direct.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.direct.xml \
	frameworks/native/data/etc/android.hardware.usb.host.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.usb.host.xml \
	frameworks/native/data/etc/android.hardware.usb.accessory.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.usb.accessory.xml \
	frameworks/native/data/etc/android.hardware.ethernet.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.ethernet.xml \
	frameworks/native/data/etc/android.software.freeform_window_management.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.software.freeform_window_management.xml \
	device/ti/am57xevm/pixcir_tangoc.idc:$(TARGET_COPY_OUT_VENDOR)/usr/idc/pixcir_tangoc.idc \

# These are the hardware-specific features
PRODUCT_COPY_FILES += \
	frameworks/native/data/etc/android.hardware.camera.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.camera.xml

# Audio
PRODUCT_COPY_FILES += \
	device/ti/am57xevm/audio/primary/mixer_paths.xml:$(TARGET_COPY_OUT_VENDOR)/etc/mixer_paths.xml \
	device/ti/am57xevm/audio/audio_policy.conf:$(TARGET_COPY_OUT_VENDOR)/etc/audio_policy.conf

# cpuset configuration
PRODUCT_COPY_FILES += \
	device/ti/am57xevm/init.am57xevmboard.cpuset.sh:$(TARGET_COPY_OUT_VENDOR)/bin/init.am57xevmboard.cpuset.sh


PRODUCT_PROPERTY_OVERRIDES := \
	hwui.render_dirty_regions=false

PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
	persist.sys.usb.config=mtp

PRODUCT_PROPERTY_OVERRIDES += \
	ro.opengles.version=131072

PRODUCT_CHARACTERISTICS := tablet,nosdcard

DEVICE_PACKAGE_OVERLAYS := \
	device/ti/am57xevm/overlay

PRODUCT_PACKAGES += \
	com.android.future.usb.accessory

PRODUCT_PROPERTY_OVERRIDES += \
	ro.sf.lcd_density=120

# WI-Fi
# PRODUCT_PACKAGES += \
	wificond \
	wpa_supplicant \
	wpa_supplicant.conf \
	wpa_supplicant_overlay.conf \
	libwpa_client \
	hostapd \
	hostapd.conf \
	dhcpcd.conf \
	wifical.sh \
	TQS_D_1.7.ini \
	TQS_D_1.7_127x.ini \
	crda \
	regulatory.bin \
	wlconf

PRODUCT_PACKAGES += \
	camera_test \
	ion_tiler_test \
	iontest \
	ion_ti_test2 \
	vpetest \
	modetest \
	libdrm

# Audio HAL modules
PRODUCT_PACKAGES += audio.primary.am57x
PRODUCT_PACKAGES += audio.hdmi.am57x
# BlueDroid a2dp Audio HAL module
PRODUCT_PACKAGES += audio.a2dp.default
# Remote submix
PRODUCT_PACKAGES += audio.r_submix.default

PRODUCT_PACKAGES += \
	tinymix \
	tinyplay \
	tinycap

# Radio
# PRODUCT_PACKAGES += \
	RadioApp \
	lad_dra7xx \
	libtiipc \
	libtiipcutils \
	libtitransportrpmsg

# Launcher3
PRODUCT_PACKAGES += Launcher3 \
		WallpaperPicker

# Enable AAC 5.1 decode (decoder)
PRODUCT_PROPERTY_OVERRIDES += \
	media.aac_51_output_enabled=true

#$(call inherit-product, frameworks/native/build/tablet-7in-hdpi-1024-dalvik-heap.mk)
$(call inherit-product-if-exists, hardware/ti/dra7xx/am57x.mk)
#$(call inherit-product-if-exists, hardware/ti/wpan/ti-wpan-products.mk)
$(call inherit-product-if-exists, device/ti/proprietary-open/jacinto6/ti-jacinto6-vendor.mk)
$(call inherit-product-if-exists, device/ti/proprietary-open/jacinto6/ducati-full_jacinto6evm.mk)
$(call inherit-product-if-exists, device/ti/proprietary-open/wl12xx/wlan/wl12xx-wlan-fw-products.mk)
$(call inherit-product-if-exists, device/ti/proprietary-open/wl12xx/wpan/wl12xx-wpan-fw-products.mk)
