#!/bin/bash

if [ `basename $PWD` != "build" ]
then
    echo "script should be called from build directory not $PWD"
fi

REQUIRED_VARIABLES_NAMES=(TENGINE_PROTOBUF_PATH TENGINE_BLAS_PATH TENGINE_ANDROID_NDK TENGINE_OPENCV_DIR)

for VARIABLE_NAME in ${REQUIRED_VARIABLES_NAMES[@]}
do
    if [ -z "${!VARIABLE_NAME}" ]
    then
        echo "$VARIABLE_NAME should be set"
        exit 1
    fi
done

cmake -DCMAKE_TOOLCHAIN_FILE=${TENGINE_ANDROID_NDK}/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI="arm64-v8a" \
    -DCONFIG_ARCH_BLAS=ON \
    -DANDROID_PLATFORM=android-21 \
    -DANDROID_STL=gnustl_shared \
    -DPROTOBUF_DIR=${TENGINE_PROTOBUF_PATH} \
    -DBLAS_DIR=${TENGINE_BLAS_PATH} \
    -DOpenCV_DIR=${TENGINE_OPENCV_DIR} \
    -DTENGINE_DIR=`dirname $PWD` \
    ..
