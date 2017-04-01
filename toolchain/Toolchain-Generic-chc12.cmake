#
# Tool chain support for Freescale (nee Metrowerks) HC12 (S12)
#
# To use this invoke CMake like so:
#
# Important, make sure you include the ":string" type option, otherwise the entries won't be placed in the
# cache causing later problems when make automatically invokes a CMake rebuild.
#
MESSAGE ("Loaded: Toolchain-Generic-chc12 .cmake")

# Look for modules in this path
SET (CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

# Path to compiler
IF (NOT DEFINED HC12_PATH)
    find_path(HC12_PATH NAMES Prog/chc12.exe
                        PATHS "/Users/joakim/.wine/drive_c/P"
                              "/c/Program Files/Freescale/CWS12v5.1/"
                              "/c/Freescale/CWS12v5.1/"
                              "/cygdrive/c/Program Files/Freescale/CWS12v5.1/"
                              "P:"
                              "C:/Program Files/Freescale/CWS12v5.1/"
                              "C:/Freescale/CWS12v5.1/"
                        NO_DEFAULT_PATH)
ENDIF()

LIST(APPEND CMAKE_PROGRAM_PATH "${HC12_PATH}/Prog")

SET (CMAKE_SYSTEM_NAME          "Generic")
SET (CMAKE_SYSTEM_VERSION       "0.0")
SET (CMAKE_SYSTEM_PROCESSOR     "hc12")

SET (HC12_CORE         "HCS12" CACHE STRING "")
SET (HC12_MEMORY_MODEL "b"     CACHE STRING "")

#
# Toolchain - Compilers, librarian (archiver) and linker.
#              Piper is a Freescale command line tool that is used to redirect their GUI tools output to stdout.
#
set(CMAKE_C_COMPILER        "chc12.exe")
set(CMAKE_C_COMPILER_ID     "chc12")
set(CMAKE_C_COMPILER_ID_RUN 1)
set(CMAKE_C_PLATFORM_ID     "")

set(CMAKE_CXX_COMPILER       "chc12.exe")
set(CMAKE_CXX_COMPILER_ID    "chc12")
set(CMAKE_CXX_COMPILER_ID_RUN 1)
set(CMAKE_CXX_PLATFORM_ID     "")
