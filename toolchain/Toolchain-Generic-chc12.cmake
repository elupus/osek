#
# Tool chain support for Freescale (nee Metrowerks) HC12 (S12)
#
# To use this invoke CMake like so:
#
# Important, make sure you include the ":string" type option, otherwise the entries won't be placed in the
# cache causing later problems when make automatically invokes a CMake rebuild.
#
INCLUDE(CMakeForceCompiler)
MESSAGE ("Loaded: Toolchain-Generic-chc12 .cmake")

# Look for modules in this path
SET (CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

# Path to compiler, must not contain spaces
IF (WIN32)
SET (HC12_PATH "P:")
ELSE()
SET (HC12_PATH "/Users/joakim/.wine/drive_c/P")
ENDIF()

LIST(APPEND CMAKE_PROGRAM_PATH "${HC12_PATH}/Prog")

SET (CMAKE_SYSTEM_NAME          "Generic")
SET (CMAKE_SYSTEM_VERSION       "0.0")
SET (CMAKE_SYSTEM_PROCESSOR     "hc12")

SET (HC12_CORE         HCS12)
SET (HC12_MEMORY_MODEL Banked)

#
# Toolchain - Compilers, librarian (archiver) and linker.
#              Piper is a Freescale command line tool that is used to redirect their GUI tools output to stdout.
#
CMAKE_FORCE_C_COMPILER  ("${HC12_PATH}/Prog/chc12.exe"     chc12)
CMAKE_FORCE_CXX_COMPILER("${HC12_PATH}/Prog/chc12.exe"     chc12)
set(CMAKE_AR             "${HC12_PATH}/Prog/libmaker.exe")
set(CMAKE_LINKER         "${HC12_PATH}/Prog/linker.exe")
