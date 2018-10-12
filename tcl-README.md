## Android Build

```bash
mkdir build
cd build
../android_build_with_blas_from_env_armv8.sh
```
You will need to setup couple environmental variables and get

- protobuf_lib
- OpenBLAS
- OpenCV

## CLion configuration

TEngine stores config of the build in the following files:

- android_config.tcl
- android_build_armv8.sh
- examples/android_build_armv8.sh

If you use CLion in order to index properly you can set values of config variables from these scripts in the CMake
profile (File | Settings | Build, Execution, Deployment | CMake). [reference](https://intellij-support.jetbrains.com/hc/en-us/community/posts/360001207879-custom-shell-script-setting-environmental-variables-with-paths-for-cmake-)

Example of variables which need to be set
```
-DPROTOBUF_DIR=/home/kamil/tools/protobuf_lib
-DCMAKE_TOOLCHAIN_FILE=/home/kamil/tools/android-ndk-r16b/build/cmake/android.toolchain.cmake
-DANDROID_ABI="arm64-v8a"
-DBLAS_DIR=/home/kamil/tools/openbla020_android
-DCONFIG_ARCH_BLAS=ON
-DOpenCV_DIR=/home/kamil/tools/OpenCV-android-sdk/sdk/native/jni
```
