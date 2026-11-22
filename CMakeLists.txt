cmake_minimum_required(VERSION 3.21)

project(OsekOs
    VERSION 1.0
    DESCRIPTION "OSEK operating system"
    LANGUAGES C
)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------

# Host tests need a host compiler and GoogleTest, so default them off when
# cross compiling (e.g. with the HC12 toolchain files).
if(CMAKE_CROSSCOMPILING)
    set(_osek_tests_default OFF)
else()
    set(_osek_tests_default ON)
endif()
option(BUILD_TESTS "Build the GoogleTest based unit tests" ${_osek_tests_default})
option(OSEK_COVERAGE "Instrument unit tests for coverage (GCC/Clang)" ON)

# Architecture port. Defaults to a sensible value for the target platform but
# can be overridden with -DBUILD_ARCH=<arch>.
if(WIN32 OR CYGWIN)
    set(_osek_arch_default Fibers)
elseif(UNIX)
    set(_osek_arch_default Posix)
elseif(HC12_CORE)
    set(_osek_arch_default HCS12)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^riscv")
    set(_osek_arch_default RiscV)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|cortex-m|thumb)")
    set(_osek_arch_default CortexM)
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "avr")
    set(_osek_arch_default Avr)
else()
    set(_osek_arch_default "")
endif()
set(BUILD_ARCH "${_osek_arch_default}" CACHE STRING "OS architecture port")
set_property(CACHE BUILD_ARCH PROPERTY STRINGS Posix Fibers Ums HCS12 RiscV CortexM Avr)

if(NOT BUILD_ARCH)
    message(FATAL_ERROR "Unable to determine BUILD_ARCH for this platform, set it with -DBUILD_ARCH=<Posix|Fibers|Ums|HCS12|RiscV|CortexM|Avr>")
endif()
message(STATUS "OSEK architecture port: ${BUILD_ARCH}")

if(BUILD_TESTS)
    enable_language(CXX)
endif()

# ---------------------------------------------------------------------------
# OS targets
#
# Os.h includes the application specific Os_Cfg.h, so the OS sources must be
# compiled once per application with that application's include path. They
# are therefore exposed as INTERFACE sources: every target linking OsekOs
# compiles its own copy against its own Os_Cfg.h.
# ---------------------------------------------------------------------------

# Headers, definitions and platform includes only.
add_library(OsekOs_Headers INTERFACE)
add_library(OsekOs::Headers ALIAS OsekOs_Headers)

target_include_directories(OsekOs_Headers INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(CMAKE_C_COMPILER_ID MATCHES "chc12")
    target_include_directories(OsekOs_Headers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include/chc)
elseif(MSVC)
    target_include_directories(OsekOs_Headers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include/msvc)
else()
    target_include_directories(OsekOs_Headers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include/gcc)
endif()

# Needed to get all stdint.h macros even in C++ translation units.
target_compile_definitions(OsekOs_Headers INTERFACE
    __STDC_LIMIT_MACROS
    __STDC_CONSTANT_MACROS
)

# Full OS: headers plus the kernel and architecture sources.
add_library(OsekOs INTERFACE)
add_library(OsekOs::OsekOs ALIAS OsekOs)
target_link_libraries(OsekOs INTERFACE OsekOs_Headers)
target_sources(OsekOs INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src/Os.c)

if(BUILD_ARCH STREQUAL "Fibers")
    enable_language(ASM_MASM)
    target_sources(OsekOs INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Os_Arch_Fibers.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Os_Arch_Fibers_Asm.asm
    )
    target_link_libraries(OsekOs INTERFACE winmm)
    target_compile_definitions(OsekOs_Headers INTERFACE OS_CFG_ARCH_FIBERS)
elseif(BUILD_ARCH STREQUAL "Ums")
    target_sources(OsekOs INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src/Os_Arch_Ums.c)
    target_compile_definitions(OsekOs_Headers INTERFACE OS_CFG_ARCH_UMS _WIN32_WINNT=0x0601)
elseif(BUILD_ARCH STREQUAL "Posix")
    target_sources(OsekOs INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src/Os_Arch_Posix.c)
    target_compile_definitions(OsekOs_Headers INTERFACE _XOPEN_SOURCE=600 OS_CFG_ARCH_POSIX)
elseif(BUILD_ARCH STREQUAL "HCS12")
    target_sources(OsekOs INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Os_Arch_HCS12.c
        ${CMAKE_CURRENT_SOURCE_DIR}/test/common/hcs12/trap.c
    )
    target_compile_definitions(OsekOs_Headers INTERFACE OS_CFG_ARCH_HCS12)
    target_link_options(OsekOs INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/test/common/hcs12/link.prm)
elseif(BUILD_ARCH STREQUAL "RiscV")
    enable_language(ASM)
    target_sources(OsekOs INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Os_Arch_RiscV.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Os_Arch_RiscV_Asm.S
    )
    target_compile_definitions(OsekOs_Headers INTERFACE OS_CFG_ARCH_RISCV)
elseif(BUILD_ARCH STREQUAL "CortexM")
    enable_language(ASM)
    # vectors.c is the vector table for picolibc's crt0, as used under QEMU
    target_sources(OsekOs INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Os_Arch_CortexM.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Os_Arch_CortexM_Asm.S
        ${CMAKE_CURRENT_SOURCE_DIR}/test/common/cortexm/vectors.c
    )
    target_compile_definitions(OsekOs_Headers INTERFACE OS_CFG_ARCH_CORTEXM)
elseif(BUILD_ARCH STREQUAL "Avr")
    enable_language(ASM)
    target_sources(OsekOs INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Os_Arch_Avr.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Os_Arch_Avr_Asm.S
    )
    target_compile_definitions(OsekOs_Headers INTERFACE OS_CFG_ARCH_AVR)
else()
    message(FATAL_ERROR "Unknown BUILD_ARCH '${BUILD_ARCH}'")
endif()

# Coverage instrumentation for unit tests, link to enable.
add_library(OsekOs_Coverage INTERFACE)
add_library(OsekOs::Coverage ALIAS OsekOs_Coverage)
if(OSEK_COVERAGE)
    set(_osek_cov $<OR:$<C_COMPILER_ID:GNU,Clang,AppleClang>,$<CXX_COMPILER_ID:GNU,Clang,AppleClang>>)
    target_compile_options(OsekOs_Coverage INTERFACE $<${_osek_cov}:--coverage -O0>)
    target_link_options(OsekOs_Coverage INTERFACE $<${_osek_cov}:--coverage>)
endif()

# ---------------------------------------------------------------------------
# Applications and tests, one directory each
# ---------------------------------------------------------------------------

# Applications run as tests on the host, or under an emulator given by the
# toolchain file (CMAKE_CROSSCOMPILING_EMULATOR) when cross compiling.
if(NOT CMAKE_CROSSCOMPILING OR CMAKE_CROSSCOMPILING_EMULATOR)
    set(_osek_app_tests_default ON)
else()
    set(_osek_app_tests_default OFF)
endif()
option(BUILD_APP_TESTS "Run the applications as tests" ${_osek_app_tests_default})

# Before the applications, so they can register tests.
if(BUILD_TESTS OR BUILD_APP_TESTS)
    enable_testing()
endif()

if(BUILD_ARCH STREQUAL "Posix")
    add_subdirectory(test/Os_Run)
    add_subdirectory(test/Os_MetricBasic)
    add_subdirectory(test/Os_MetricCooperative)
    add_subdirectory(test/Os_MetricPreemptive)
    add_subdirectory(test/Os_MetricEvents)
elseif(BUILD_ARCH STREQUAL "Fibers")
    add_subdirectory(test/Os_Run)
    add_subdirectory(test/Os_MetricBasic)
    add_subdirectory(test/Os_MetricCooperative)
    add_subdirectory(test/Os_MetricPreemptive)
    add_subdirectory(test/Os_MetricEvents)
elseif(BUILD_ARCH STREQUAL "Ums")
    add_subdirectory(test/Os_Run)
elseif(BUILD_ARCH STREQUAL "RiscV")
    add_subdirectory(test/Os_Run)
    add_subdirectory(test/Os_MetricBasic)
    add_subdirectory(test/Os_MetricCooperative)
    add_subdirectory(test/Os_MetricPreemptive)
    add_subdirectory(test/Os_MetricEvents)
elseif(BUILD_ARCH STREQUAL "CortexM")
    add_subdirectory(test/Os_Run)
    add_subdirectory(test/Os_MetricBasic)
    add_subdirectory(test/Os_MetricCooperative)
    add_subdirectory(test/Os_MetricPreemptive)
    add_subdirectory(test/Os_MetricEvents)
elseif(BUILD_ARCH STREQUAL "HCS12")
    add_subdirectory(test/Os_HCS12)
    add_subdirectory(test/Os_MetricBasic)
    add_subdirectory(test/Os_MetricCooperative)
    add_subdirectory(test/Os_MetricPreemptive)
endif()

if(BUILD_TESTS)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    add_subdirectory(externals/googletest EXCLUDE_FROM_ALL)

    include(GoogleTest)

    add_subdirectory(test/Os_Test)
    add_subdirectory(test/Os_TestInternal)
endif()

# ---------------------------------------------------------------------------
# Per-source options for the OS sources
#
# Source file properties are scoped to a directory, and INTERFACE sources are
# compiled in the directory of the consuming target, so apply them to every
# subdirectory that may build the OS.
# ---------------------------------------------------------------------------

get_property(_osek_dirs DIRECTORY PROPERTY SUBDIRECTORIES)
list(APPEND _osek_dirs ${CMAKE_CURRENT_SOURCE_DIR})
if(BUILD_ARCH STREQUAL "Posix")
    # ucontext functions are marked deprecated by POSIX but still required.
    set_source_files_properties(src/Os_Arch_Posix.c DIRECTORY ${_osek_dirs}
        PROPERTIES COMPILE_OPTIONS "-Wno-deprecated-declarations")
elseif(BUILD_ARCH STREQUAL "Fibers" AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    # SAFESEH only exists for 32 bit, ml64 rejects the option.
    set_source_files_properties(src/Os_Arch_Fibers_Asm.asm DIRECTORY ${_osek_dirs}
        PROPERTIES COMPILE_OPTIONS "-safeseh")
endif()
