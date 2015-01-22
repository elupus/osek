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

LIST(APPEND CMAKE_PROGRAM_PATH "C:/Program Files (x86)/Freescale/CWS12v5.1/Prog")

SET (HC12_LIB_SEARCH_DIR "C:/Program Files (x86)/Freescale/CWS12v5.1/lib/hc12c/lib")

SET (CMAKE_SYSTEM_NAME          "Freescale")
SET (CMAKE_SYSTEM_VERSION       "0.0")
SET (CMAKE_SYSTEM_PROCESSOR     "hc12")

#
# Toolchain - Compilers, librarian (archiver) and linker.
#              Piper is a Freescale command line tool that is used to redirect their GUI tools output to stdout.
#
find_program(HC12_PIPER         NAMES piper)
find_program(CMAKE_C_COMPILER   NAMES chc12)
find_program(CMAKE_CXX_COMPILER NAMES chc12)
find_program(CMAKE_AR           NAMES libmaker)
find_program(CMAKE_LINKER       NAMES linker)

#
# We have to setup the object type very early, as this can't be overridden later on.
#
SET (CMAKE_C_OUTPUT_EXTENSION     ".o"         CACHE STRING "C compiler object extension.")
SET (CMAKE_CXX_OUTPUT_EXTENSION   ".o"         CACHE STRING "C++ compiler object extension.")

#
# Convince CMake we know our compiler works OK.
# If we don't do this it will do a TRY-COMPILE which will fail to link.
#
SET (CMAKE_C_COMPILER_ID_RUN    TRUE)
SET (CMAKE_C_COMPILER_WORKS     TRUE)
SET (CMAKE_CXX_COMPILER_ID_RUN  TRUE)
SET (CMAKE_CXX_COMPILER_WORKS   TRUE)

#
# Workaround to inhibit CMake from performing a TRY-COMPILE to determine the size of "void *",
# this size isn't ever used so its actual value doesn't matter.
#
SET (CMAKE_SIZEOF_VOID_P 2)

#
# Don't generate preprocessed or assembler makefile rules for C/C++ source files.
#
SET (CMAKE_SKIP_PREPROCESSED_SOURCE_RULES    TRUE)
SET (CMAKE_SKIP_ASSEMBLY_SOURCE_RULES        TRUE)
