In the current setup we use dynamic linking so you need to copy couple so files into the directory where benchmark app
is stored:

- libtengine.so
- libprotobuf.so - probably from location like protobuf_lib/arm64_lib/libprotobuf.so
- libopenblas.so - probably from location like OpenBLAS_android/lib/libopenblas.so
- libgnustl_shared.so - probably from location like
    android-ndk-r16b/sources/cxx-stl/gnu-libstdc++/4.9/libs/arm64-v8a/libgnustl_shared.so
    
Also we need the model to run so from project Inference directory Inference/resources/lfs we need
- FSRCNN-s_net.prototxt 
- TCL_s_x2_denoise_v4.caffemodel

After collecting these files in the directory like /data/local/tmp/Tengine we can run the commands
```bash

export LD_LIBRARY_PATH='.'
SLEEP_TIME=0 OPENBLAS_NUM_THREADS=4 REPEAT_COUNT=4 ./benchmark_sr_runtimes_only
```
Tengine has the option to set active cores and it's available in benchmark_sr.
Library itself has some debug output which is printed apart from lines with time and threads number so the output looks
like

```
/data/local/tmp/Tengine/benchmark_sr_runtimes_only
tensor: conv3 created by node: conv3 is not consumed
add the node: conv3 into output list
1193.120972	4
1086.838989	4
1032.850952	4
1010.419006	4
Release Graph Executor for graph graph
Release workspace default resource

```
