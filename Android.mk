LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libwait
LOCAL_SRC_FILES := src/callout.cpp src/module.cpp \
				   src/utils.cpp src/slotwait.cpp \
				   src/platform-linux.cpp src/slotsock-linux.cpp
LOCAL_C_INCLUDES += $(JNI_H_INCLUDE) $(LOCAL_PATH)/include
LOCAL_CXX_INCLUDES += $(JNI_H_INCLUDE) $(LOCAL_PATH)/include
include $(BUILD_STATIC_LIBRARY)

