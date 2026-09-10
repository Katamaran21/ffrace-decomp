LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../SDL

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include

# ff_platform_win32.c and ff_audio_win32.c are the native Windows/CE
# backend and include <windows.h>, which the NDK has no notion of.
LOCAL_SRC_FILES := $(filter-out %_win32.c,$(notdir $(wildcard $(LOCAL_PATH)/*.c)))

LOCAL_SHARED_LIBRARIES := SDL2

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -lOpenSLES -llog -landroid -lm

include $(BUILD_SHARED_LIBRARY)
