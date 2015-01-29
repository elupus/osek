#
# Tool chain support for Freescale (nee Metrowerks) HC12 (S12)
#
# To use this invoke CMake like so:
#
#    CMake -G"NMake Makefiles" -DCMAKE_MODULE_PATH:string=c:/freescale-hc12-demo/toolchain -DCMAKE_TOOLCHAIN_FILE:string=Toolchain-Freescale-HC12 ..\source
#
# Important, make sure you include the ":string" type option, otherwise the entries won't be placed in the
# cache causing later problems when make automatically invokes a CMake rebuild.
#

MESSAGE ("Loaded: Toolchain-Freescale-HC12.cmake")

# Look for modules in this path
SET (CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

# Path to compiler, must not contain spaces
SET (HC12_PATH "P:")

LIST(APPEND CMAKE_PROGRAM_PATH "${HC12_PATH}/Prog")

SET (CMAKE_SYSTEM_NAME          "Freescale")
SET (CMAKE_SYSTEM_VERSION       "0.0")
SET (CMAKE_SYSTEM_PROCESSOR     "hc12")

#
# Toolchain - Compilers, librarian (archiver) and linker.
#              Piper is a Freescale command line tool that is used to redirect their GUI tools output to stdout.
#
set(HC12_PIPER         ${HC12_PATH}/Prog/piper)
set(HC12_BURNER        ${HC12_PATH}/Prog/burner.exe)
set(CMAKE_C_COMPILER   ${HC12_PATH}/Prog/chc12.exe)
set(CMAKE_CXX_COMPILER ${HC12_PATH}/Prog/chc12.exe)
set(CMAKE_AR           ${HC12_PATH}/Prog/libmaker.exe)
set(CMAKE_LINKER       ${HC12_PATH}/Prog/linker.exe)

set(CMAKE_OBJCOPY      FALSE)
set(CMAKE_OBJDUMP      FALSE)

include_directories(${HC12_PATH}/lib/hc12c/include)
link_directories(${HC12_PATH}/lib/hc12c/lib)
set (HC12_LINK_PRM_FILE ${HC12_PATH}/lib/hc12c/prm/mc9s12e128.prm)
