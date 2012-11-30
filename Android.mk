LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libwait
LOCAL_SRC_FILES := src/callout.cpp src/module.cpp \
				   src/utils.cpp src/slotwait.cpp \
				   src/platform-linux.cpp src/slotsock-linux.cpp \
LOCAL_CFLAGS  := -Iinclude
LOCAL_CXXFLAGS  := -Iinclude
include $(BUILD_STATIC_LIBRARY)

