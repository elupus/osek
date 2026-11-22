# Eclipse ThreadX versions of the OSEK metric apps, for comparison on
# RISC-V under QEMU. A standalone project, not part of the main build.
# Configure it with the same toolchain file as the riscv32-qemu preset:
#
#   cmake -S benchmarks/threadx -B build/bench-threadx -G Ninja \
#         -DCMAKE_TOOLCHAIN_FILE=toolchain/Toolchain-riscv32-elf.cmake
#
# benchmarks/compare.sh finds this project, builds it and runs its apps
# next to the OSEK ones.

cmake_minimum_required(VERSION 3.21)

project(ThreadXMetrics
    DESCRIPTION "Eclipse ThreadX versions of the OSEK metric apps"
    LANGUAGES C ASM
)

if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^riscv32")
    message(FATAL_ERROR "Configure with -DCMAKE_TOOLCHAIN_FILE=toolchain/Toolchain-riscv32-elf.cmake")
endif()

# Same flags as the riscv32-qemu preset of the OSEK build (-Os -DNDEBUG).
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE MinSizeRel CACHE STRING "Build type" FORCE)
endif()

# ThreadX is fetched, not vendored. SOURCE_SUBDIR names a directory without
# a CMakeLists.txt so that FetchContent only downloads the sources, and the
# ones needed are compiled below with the toolchain file's flags. Set
# FETCHCONTENT_SOURCE_DIR_THREADX to use a local checkout instead.
include(FetchContent)
FetchContent_Declare(threadx
    GIT_REPOSITORY https://github.com/eclipse-threadx/threadx.git
    GIT_TAG        v6.5.2.202603b_rel
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  do-not-add
)
FetchContent_MakeAvailable(threadx)
set(_threadx_dir ${threadx_SOURCE_DIR})
set(_threadx_port ${_threadx_dir}/ports/risc-v32/gnu)

# The generic sources and the RV32 GNU port, all built with tx_user.h. A
# static library, so that each app only links the services it uses. The
# port's own _tx_initialize_low_level is left out: ThreadX_Metric_Port.S
# has the one for QEMU virt.
file(GLOB _threadx_sources ${_threadx_dir}/common/src/tx_*.c)
add_library(ThreadX_Kernel STATIC
    ${_threadx_sources}
    ${_threadx_port}/src/tx_thread_context_restore.S
    ${_threadx_port}/src/tx_thread_context_save.S
    ${_threadx_port}/src/tx_thread_interrupt_control.S
    ${_threadx_port}/src/tx_thread_schedule.S
    ${_threadx_port}/src/tx_thread_stack_build.S
    ${_threadx_port}/src/tx_thread_system_return.S
    ${_threadx_port}/src/tx_timer_interrupt.S
)
target_include_directories(ThreadX_Kernel PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${_threadx_dir}/common/inc
    ${_threadx_port}/inc
)
target_compile_definitions(ThreadX_Kernel PUBLIC TX_INCLUDE_USER_DEFINE_FILE)

# Start up, trap handler and tick for QEMU virt, shared by the apps.
add_library(ThreadX_Metric OBJECT ThreadX_Metric_Port.S)
target_link_libraries(ThreadX_Metric PUBLIC ThreadX_Kernel)

# ---------------------------------------------------------------------------
# Applications, one directory each. ThreadX_Metric<Name> is the counterpart
# of the OSEK app test/Os_Metric<Name>.
# ---------------------------------------------------------------------------

add_subdirectory(ThreadX_MetricBasic)
add_subdirectory(ThreadX_MetricCooperative)
add_subdirectory(ThreadX_MetricPreemptive)
add_subdirectory(ThreadX_MetricEvents)
