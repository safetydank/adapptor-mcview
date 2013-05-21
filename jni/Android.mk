LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := mcview

LOCAL_C_INCLUDES := $(LOCAL_PATH)/mcview
LOCAL_SRC_FILES  := mcview/MCNative.cpp

LOCAL_LDLIBS := -landroid -ljnigraphics -lOpenSLES
LOCAL_STATIC_LIBRARIES := motioncel \
                          cinder-es2 \
	                      boost_system boost_filesystem boost_thread \
						  ft2 \
						  cpufeatures \
						  minizip

include $(BUILD_STATIC_LIBRARY)

$(call import-module,motioncel)
$(call import-module,cinder-es2)
$(call import-module,boost)
$(call import-module,minizip)
$(call import-module,android/cpufeatures)

