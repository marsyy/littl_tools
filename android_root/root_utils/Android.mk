LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	get_root.c

LOCAL_C_INCLUDES := \
	bionic \

ifeq ($(shell expr $(PLATFORM_SDK_VERSION) "<" 23), 1)
	LOCAL_SHARED_LIBRARIES += libstlport
	LOCAL_C_INCLUDES += \
						bionic/libstdc++/include \
						external/stlport/stlport
endif

LOCAL_MODULE	:= get_root

include $(BUILD_STATIC_LIBRARY)