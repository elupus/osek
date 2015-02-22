cmake_minimum_required (VERSION 2.8.4)
project (OsekOs C)

if(BUILD_TESTS)
	enable_language(CXX)
endif()

set (OsekOs_VERSION_MAJOR 1)
set (OsekOs_VERSION_MINOR 0)

set (Os_SRCS
    src/Os.c
)

if (WIN32 OR CYGWIN)
	enable_language(ASM)
    set (Os_SRCS ${Os_SRCS}
        src/Os_Arch_Fibers.c
        src/Os_Arch_Fibers.S
    )
    add_definitions(-DOS_CFG_ARCH_FIBERS)
    set (Os_Run 1)
elseif (UNIX)
    set (Os_SRCS ${Os_SRCS}
        src/Os_Arch_Posix.c
    )
    add_definitions(-D_XOPEN_SOURCE=600 -DOS_CFG_ARCH_POSIX)
    set (Os_Run 1)
elseif (HC12_CORE)
    set (Os_SRCS ${Os_SRCS}
        src/Os_Arch_HCS12.c
    )
    add_definitions(-DOS_CFG_ARCH_HCS12)
    set (Os_HCS12 1)
endif()

include_directories(include src)

if (CMAKE_C_COMPILER_ID MATCHES chc12)
    include_directories(include/chc)
else()
    include_directories(include/gcc)
endif()

set_property(
    SOURCE src/Os_Arch_Posix.c
    PROPERTY COMPILE_FLAGS -Wno-deprecated-declarations
)

option(BUILD_TESTS "Enable Testing" ON)

if(BUILD_TESTS)
	include(ExternalProject)
	ExternalProject_Add(googletest
	  URL https://googletest.googlecode.com/files/gtest-1.7.0.zip
	  # Comment above line, and uncomment line below to use subversion.
	  # SVN_REPOSITORY http://googletest.googlecode.com/svn/trunk/
	  # Uncomment line below to freeze a revision (here the one for 1.7.0)
	  # SVN_REVISION -r700

	  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/googletest
	  INSTALL_COMMAND ""
	)
	# Specify include dir
	ExternalProject_Get_Property(googletest source_dir)
	set(GTEST_INCLUDE_DIR ${source_dir}/include)

	# Library
	ExternalProject_Get_Property(googletest binary_dir)

	set(GTEST_LIBRARY_PATH "${binary_dir}/libgtest.a")
	add_library(gtest UNKNOWN IMPORTED)
	set_property(TARGET gtest PROPERTY IMPORTED_LOCATION ${GTEST_LIBRARY_PATH} )
	add_dependencies(gtest googletest)

	set(GTEST_MAIN_PATH "${binary_dir}/libgtest_main.a")
	add_library(gtest_main UNKNOWN IMPORTED)
	set_property(TARGET gtest_main PROPERTY IMPORTED_LOCATION ${GTEST_MAIN_PATH} )
	add_dependencies(gtest_main googletest)

	# GTest type test
	add_executable(Os_Test ${Os_SRCS} test/Os_Test/Os_Test.cpp)
	target_include_directories(Os_Test PRIVATE test/Os_Test ${GTEST_INCLUDE_DIR})
	target_link_libraries(Os_Test gtest gtest_main pthread)

	set_target_properties(Os_Test PROPERTIES COMPILE_FLAGS "--coverage -O0")
	set_target_properties(Os_Test PROPERTIES LINK_FLAGS    "--coverage -O0")

	# GTest type test
	add_executable(Os_TestInternal test/Os_TestInternal/Os_TestInternal.cpp)
	target_include_directories(Os_TestInternal PRIVATE test/Os_TestInternal ${GTEST_INCLUDE_DIR})
	target_link_libraries(Os_TestInternal gtest gtest_main pthread)

	set_target_properties(Os_TestInternal PROPERTIES COMPILE_FLAGS "--coverage -O0")
	set_target_properties(Os_TestInternal PROPERTIES LINK_FLAGS    "--coverage -O0")

endif()

if(${Os_Run})
    # Simple Os_Run test
    add_executable(Os_Run ${Os_SRCS} test/Os_Run/Os_Cfg.c)
    target_include_directories(Os_Run PRIVATE test/Os_Run)
endif()

if(${Os_HCS12})
    # Simple Os_Run test
    add_executable(Os_HCS12 ${Os_SRCS} test/Os_HCS12/Os_Cfg.c)
    if (CMAKE_C_COMPILER_ID MATCHES chc12)
        set_target_properties(Os_HCS12 PROPERTIES LINK_FLAGS ${CMAKE_CURRENT_SOURCE_DIR}/test/Os_HCS12/link.prm)
    endif()
    target_include_directories(Os_HCS12 PRIVATE test/Os_HCS12)
    if(CMAKE_OBJCOPY AND CMAKE_OBJDUMP)
        add_custom_command(TARGET Os_HCS12 POST_BUILD
                           COMMAND ${CMAKE_OBJCOPY} --output-format=srec $<TARGET_FILE:Os_HCS12> Os_HCS12.s19
                           COMMAND ${CMAKE_OBJDUMP} $<TARGET_FILE:Os_HCS12> -S --disassemble   > Os_HCS12.lst)
    endif()
    if(CHC12_BURNER)
        add_custom_command(TARGET Os_HCS12 POST_BUILD
                           COMMAND ${CHC12_PIPER} ${CHC12_BURNER} -ViewHidden -EnvABS_FILE=$<TARGET_FILE:Os_HCS12> -F ${CMAKE_CURRENT_SOURCE_DIR}/test/Os_HCS12/burner.bbl)
    endif()
endif()


