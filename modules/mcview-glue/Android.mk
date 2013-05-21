LOCAL_PATH := $(call my-dir)/../../jni

include $(CLEAR_VARS)
LOCAL_MODULE := mcview-glue
LOCAL_SRC_FILES := ../obj/local/$(TARGET_ARCH_ABI)/libmcview-glue.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)

include $(PREBUILT_STATIC_LIBRARY)

