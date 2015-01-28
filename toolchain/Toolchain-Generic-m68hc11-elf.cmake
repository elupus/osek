#INCLUDE(CMakeForceCompiler)

set(CMAKE_SYSTEM_NAME      Generic)
set(CMAKE_SYSTEM_PROCESSOR hcs12)
set(CMAKE_C_COMPILER       /opt/cross/m68hc11-elf/bin/m68hc11-elf-gcc${CMAKE_EXECUTABLE_SUFFIX})

set(CMAKE_FIND_ROOT_PATH   /opt/cross/m68hc11-elf/bin)

# search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)

# for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(HCS12 1)

set(CMAKE_C_LINK_EXECUTABLE
    "<CMAKE_C_COMPILER> <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS>  -o <TARGET> -Wl,-Map,<TARGET_BASE>.map <LINK_LIBRARIES>"
    "<CMAKE_OBJCOPY> <TARGET> --output-format=srec <TARGET_BASE>.s19"
    "<CMAKE_OBJDUMP> <TARGET> -S --disassemble   > <TARGET_BASE>.lst"
)

set(CMAKE_C_FLAGS "-mshort -ffixed-z -m68hcs12 -Os -fomit-frame-pointer")
