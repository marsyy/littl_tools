LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	2.c \


LOCAL_SHARED_LIBRARIES := \
	liblog \

LOCAL_STATIC_LIBRARIES := get_root\

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../root_utils \
    $(LOCAL_PATH)/ \
	bionic \

ifeq ($(shell expr $(PLATFORM_SDK_VERSION) "<" 23), 1)
	LOCAL_SHARED_LIBRARIES += libstlport
	LOCAL_C_INCLUDES += \
		bionic/libstdc++/include \
		external/stlport/stlport
endif

LOCAL_C_INCLUDES += \

LOCAL_MODULE:= testB

include $(BUILD_EXECUTABLE)