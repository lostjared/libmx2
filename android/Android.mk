LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../SDL
APP_ALLOW_MISSING_DEPS := true

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include 
LOCAL_CPPFLAGS := -std=c++17 -DWITH_MIXER
LOCAL_CPP_FEATURES += exceptions rtti

# Add your application source files here...
LOCAL_SRC_FILES := space.cpp intro.cpp explode.cpp model.cpp gl.cpp cfg.cpp loadpng.cpp exception.cpp font.cpp input.cpp joystick.cpp mx.cpp sound.cpp tee_stream.cpp texture.cpp util.cpp

LOCAL_SHARED_LIBRARIES := SDL2 SDL2_ttf SDL2_image SDL2_mixer

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv3 -lEGL -lOpenSLES -llog -landroid -lc++_static -lc++abi

include $(BUILD_SHARED_LIBRARY)
