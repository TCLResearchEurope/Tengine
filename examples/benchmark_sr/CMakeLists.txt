cmake_minimum_required (VERSION 2.8)
project(BENCHMARK_SR)

set(INSTALL_DIR ${TENGINE_DIR}/install/)
set(TENGINE_LIBS tengine)
set(BENCHMARK_SR_RUNTIMES_ONLY_SRCS benchmark_sr_runtimes_only.cpp ../common/common.cpp)

include_directories(${TENGINE_DIR}/include
                    # needed for common_util.hpp which defines TEngine::set_cpu_list
                    ${TENGINE_DIR}/tests/include
                    ${TENGINE_DIR}/examples/common)

add_executable(benchmark_sr_runtimes_only ${BENCHMARK_SR_RUNTIMES_ONLY_SRCS})
target_link_libraries(benchmark_sr_runtimes_only ${TENGINE_LIBS})
