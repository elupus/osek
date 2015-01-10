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

add_executable(OsekOs ${FILES})
include_directories(src)

set_property(
    SOURCE src/Os_Arch_Posix.c
    PROPERTY COMPILE_FLAGS -Wno-deprecated-declarations
)


