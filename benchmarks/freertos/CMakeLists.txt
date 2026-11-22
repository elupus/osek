# FreeRTOS versions of the OSEK metric apps, for comparison on RISC-V under
# QEMU. A standalone project, not part of the main build. Configure it with
# the same toolchain file as the riscv32-qemu preset:
#
#   cmake -S benchmarks/freertos -B build/bench-freertos -G Ninja \
#         -DCMAKE_TOOLCHAIN_FILE=toolchain/Toolchain-riscv32-elf.cmake
#
# benchmarks/compare.sh finds this project, builds it and runs its apps
# next to the OSEK ones.

cmake_minimum_required(VERSION 3.21)

project(FreeRTOSMetrics
    DESCRIPTION "FreeRTOS versions of the OSEK metric apps"
    LANGUAGES C ASM
)

if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^riscv32")
    message(FATAL_ERROR "Configure with -DCMAKE_TOOLCHAIN_FILE=toolchain/Toolchain-riscv32-elf.cmake")
endif()

# Same flags as the riscv32-qemu preset of the OSEK build (-Os -DNDEBUG).
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE MinSizeRel CACHE STRING "Build type" FORCE)
endif()

# FreeRTOS is fetched, not vendored. SOURCE_SUBDIR names a directory without
# a CMakeLists.txt so that FetchContent only downloads the kernel and the
# sources needed are compiled below. Set FETCHCONTENT_SOURCE_DIR_FREERTOS_KERNEL
# to use a local checkout instead.
include(FetchContent)
FetchContent_Declare(freertos_kernel
    GIT_REPOSITORY https://github.com/FreeRTOS/FreeRTOS-Kernel.git
    GIT_TAG        V11.1.0
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  do-not-add
)
FetchContent_MakeAvailable(freertos_kernel)
set(_freertos_dir ${freertos_kernel_SOURCE_DIR})

# Only the kernel sources the apps need. All apps share FreeRTOSConfig.h,
# so the kernel is compiled once.
add_library(FreeRTOS_Kernel OBJECT
    ${_freertos_dir}/tasks.c
    ${_freertos_dir}/list.c
    ${_freertos_dir}/queue.c
    ${_freertos_dir}/portable/MemMang/heap_4.c
    ${_freertos_dir}/portable/GCC/RISC-V/port.c
    ${_freertos_dir}/portable/GCC/RISC-V/portASM.S
)
target_include_directories(FreeRTOS_Kernel PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${_freertos_dir}/include
    ${_freertos_dir}/portable/GCC/RISC-V
    ${_freertos_dir}/portable/GCC/RISC-V/chip_specific_extensions/RISCV_MTIME_CLINT_no_extensions
)

# ---------------------------------------------------------------------------
# Applications, one directory each. FreeRTOS_Metric<Name> is the counterpart
# of the OSEK app test/Os_Metric<Name>.
# ---------------------------------------------------------------------------

add_subdirectory(FreeRTOS_MetricBasic)
add_subdirectory(FreeRTOS_MetricCooperative)
add_subdirectory(FreeRTOS_MetricPreemptive)
add_subdirectory(FreeRTOS_MetricEvents)
