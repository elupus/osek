MESSAGE ("Loaded: Toolchain-Generic-HC12.cmake")

# Look for modules in this path
SET (CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

set(CMAKE_SYSTEM_NAME      Generic)
set(CMAKE_SYSTEM_PROCESSOR hc12)
set(CMAKE_C_COMPILER       /opt/cross/m68hc11-elf/bin/m68hc11-elf-gcc${CMAKE_EXECUTABLE_SUFFIX})

set(CMAKE_FIND_ROOT_PATH   /opt/cross/m68hc11-elf/bin)

# search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)

# for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
