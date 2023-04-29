cmake_minimum_required(VERSION 3.4.1)


set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread -g")#-g -O0

include_directories(${CMAKE_SOURCE_DIR}/3rdparty/opensmile/include/ ${CMAKE_SOURCE_DIR}/3rdparty/tensorflow/include/ ${CMAKE_SOURCE_DIR}/3rdparty/opencv/include/)

set(OPENSMILE_DIR_LIB ${CMAKE_SOURCE_DIR}/3rdparty/opensmile/${CMAKE_ANDROID_ARCH_ABI})

set(TENSORFLOW_DIR_LIB ${CMAKE_SOURCE_DIR}/3rdparty/tensorflow/${CMAKE_ANDROID_ARCH_ABI})

set(OPENCV_DIR_LIB ${CMAKE_SOURCE_DIR}/3rdparty/opencv/${CMAKE_ANDROID_ARCH_ABI})

add_library(libopensmile STATIC IMPORTED)
set_target_properties(libopensmile PROPERTIES IMPORTED_LOCATION
        ${OPENSMILE_DIR_LIB}/libopensmile.a)

add_library(libtensorflowLite SHARED IMPORTED)
set_target_properties(libtensorflowLite PROPERTIES IMPORTED_LOCATION
        ${TENSORFLOW_DIR_LIB}/libtensorflowLite.so)

add_library(libopencv SHARED IMPORTED)
#引入libopencv_java3.so文件
set_target_properties(libopencv PROPERTIES IMPORTED_LOCATION
        ${OPENCV_DIR_LIB}/libopencv_java3.so)

find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log )

# demo：c++的MFCC梅尔特征提取测试
add_library(speechEmotionRecognition
        SHARED
        featSMILExtract.cpp
        speechEmotionRecognitonJni.cpp
        inference.cpp
        )

target_link_libraries(speechEmotionRecognition
        libopensmile
        libtensorflowLite
        android
        ${log-lib})

