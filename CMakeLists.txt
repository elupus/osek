cmake_minimum_required (VERSION 2.6)
project (OsekOs)

set (OsekOs_VERSION_MAJOR 1)
set (OsekOs_VERSION_MINOR 0)

set (FILES
    src/Os.c
    src/Os_Cfg.c
)

if (${UNIX})
    set (FILES ${FILES}
        src/Os_Arch_Posix.c
    )
    add_definitions(-D_XOPEN_SOURCE=600 -DOS_CFG_ARCH_POSIX)
elseif (${WIN32})
    set (FILES ${FILES}
        src/Os_Arch_Fibers.c
    )
    add_definitions(-DOS_CFG_ARCH_FIBERS)
endif()

add_executable(osekos ${FILES})
include_directories(src)

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

	set(GTEST_LIBRARY_PATH ${binary_dir}/${CMAKE_FIND_LIBRARY_PREFIXES}gtest.a)
	add_library(gtest UNKNOWN IMPORTED)
	set_property(TARGET gtest PROPERTY IMPORTED_LOCATION ${GTEST_LIBRARY_PATH} )
	add_dependencies(gtest googletest)

	set(GTEST_MAIN_PATH ${binary_dir}/${CMAKE_FIND_LIBRARY_PREFIXES}gtest_main.a)
	add_library(gtest_main UNKNOWN IMPORTED)
	set_property(TARGET gtest_main PROPERTY IMPORTED_LOCATION ${GTEST_MAIN_PATH} )
	add_dependencies(gtest_main googletest)

	add_subdirectory(test)
endif()



