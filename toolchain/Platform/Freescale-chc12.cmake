#
# Generic Tool chain support.
#

MESSAGE ("Loaded: Freescale-chc12.cmake")

#
# Cross compiler generic support for Freescale (nee Metrowerks) HC12 (S12)
#
# Special variables introduced by this module, these can be overridden:
#
# HC12_CORE					: Processor core type, one of 'HC12', 'HCS12' or 'HCS12X' (case sensitive!)
# HC12_MEMORY_MODEL			: Memory model, one of 'Small', 'Large' or 'Banked' (case sensitive!)
#
# By default the various output files are placed in subdirectories of CMAKE_BINARY_DIR,
# currently: image, lst, err, lib
#
# HC12_LINK_PRM_FILE		: Input linker prm file (controls memory/segment allocation, see Freescale compiler manual).
# HC12_LIB_SEARCH_DIR		: Default user directory to search for libraries.
# HC12_LINK_OUTPUT_DIR		: Output directory for '.elf' and map etc.
# HC12_LST_OUTPUT_DIR		: Output directory for '.lst' assembler list files.
# HC12_ERR_OUTPUT_DIR		: Output directory for '.err' error files.
#
# Internally set variables (based on user specified values above):
#
# HC12_CORE_LETTER			: Automatically set based on HC12_CORE.
# HC12_MEMORY_MODEL_LETTER	: Automatically set based on HC12_MEMORY_MODEL.
# HC12_START12				: Startup object file (start12*.o) based on core and memory model.
# HC12_RTL_LIBS				: Run-time libraries based on core and memory model.
#

#
# Allow user to easily override the variables from a file rather than the CMake command line.
#
INCLUDE ("Freescale-HC12-opt.cmake" OPTIONAL RESULT_VARIABLE _INCLUDED_FREESCALE_HC12_OPT_FILE)
IF (NOT _INCLUDED_FREESCALE_HC12_OPT_FILE)
	INCLUDE ("${CMAKE_BINARY_DIR}/Freescale-HC12-opt.cmake" OPTIONAL RESULT_VARIABLE _INCLUDED_FREESCALE_HC12_OPT_FILE)
ENDIF (NOT _INCLUDED_FREESCALE_HC12_OPT_FILE)

#
# Core type (default is 'HC12'), override this on the CMake command line, e.g. "-DHC12_CORE=HCS12X".
#
IF (NOT DEFINED HC12_CORE)
	SET (HC12_CORE "HC12" CACHE STRING "HC12 core - one of 'HC12', 'HCS12' or 'HCS12X' (case sensitive!)")
ENDIF (NOT DEFINED HC12_CORE)

IF (${HC12_CORE} STREQUAL "HC12")
	SET (HC12_CORE_LETTER "")
ELSEIF (${HC12_CORE} STREQUAL "HCS12")
	SET (HC12_CORE_LETTER "")
ELSEIF (${HC12_CORE} STREQUAL "HCS12X")
	SET (HC12_CORE_LETTER 	"x")
	SET (CMAKE_C_FLAGS_INIT	"${CMAKE_C_FLAGS_INIT} -CpuHCS12X")
ELSE (${HC12_CORE} STREQUAL "HCS12X")
   	MESSAGE (FATAL_ERROR "HC12_CORE should be set to one of 'HC12', 'HCS12' or 'HCS12X' (case sensitive!)")
ENDIF (${HC12_CORE} STREQUAL "HC12")

#
# Memory model (default is 'Small'), override this on the CMake command line, e.g "-DHC12_MEMORY_MODEL=Banked".
#
IF (NOT DEFINED HC12_MEMORY_MODEL)
	SET (HC12_MEMORY_MODEL "Small" CACHE STRING "HC12 memory model - one of 'Small', 'Large' or 'Banked' (case sensitive!)")
ENDIF (NOT DEFINED HC12_MEMORY_MODEL)

IF (${HC12_MEMORY_MODEL} STREQUAL "Small")
	#
	# Small.
	#
	SET (HC12_MEMORY_MODEL_LETTER "s")
ELSEIF (${HC12_MEMORY_MODEL} STREQUAL "Large")
	#
	# Large.
	#
	SET (HC12_MEMORY_MODEL_LETTER "l")
ELSEIF (${HC12_MEMORY_MODEL} STREQUAL "Banked")
	#
	# Banked
	#
	SET (HC12_MEMORY_MODEL_LETTER "b")
ELSE (${HC12_MEMORY_MODEL} STREQUAL "Banked")
   	MESSAGE (FATAL_ERROR "HC12_MEMORY_MODEL should be set to one of 'Small', 'Large' or 'Banked' (case sensitive!)")
ENDIF (${HC12_MEMORY_MODEL} STREQUAL "Small")

SET (CMAKE_C_FLAGS_INIT	"${CMAKE_C_FLAGS_INIT} -M${HC12_MEMORY_MODEL_LETTER}")

#
# Starup code and RTL.
#
# TODO: Add C++ support.
#
SET (HC12_START12 	"start12${HC12_CORE_LETTER}${HC12_MEMORY_MODEL_LETTER}.o" CACHE STRING "HC12 startup object")
SET (HC12_RTL_LIBS  "ansi${HC12_CORE_LETTER}${HC12_MEMORY_MODEL_LETTER}.lib" CACHE STRING "HC12 run-time libraries")

#
# Input control files.
#
IF (NOT DEFINED HC12_LINK_PRM_FILE)
	SET (HC12_LINK_PRM_FILE "${CMAKE_CURRENT_SOURCE_DIR}/link.prm" CACHE STRING "HC12 linker prm file")
ENDIF (NOT DEFINED HC12_LINK_PRM_FILE)

#
# Output directories.
#
IF (NOT DEFINED HC12_LIB_SEARCH_DIR)
	SET (HC12_LIB_SEARCH_DIR "${CMAKE_BINARY_DIR}/lib" CACHE STRING "HC12 library search directory")
ENDIF (NOT DEFINED HC12_LIB_SEARCH_DIR)

IF (NOT DEFINED HC12_LINK_OUTPUT_DIR)
	SET (HC12_LINK_OUTPUT_DIR "${CMAKE_BINARY_DIR}/image" CACHE STRING "HC12 linker output directory")
ENDIF (NOT DEFINED HC12_LINK_OUTPUT_DIR)

IF (NOT DEFINED HC12_LST_OUTPUT_DIR)
	SET (HC12_LST_OUTPUT_DIR "${CMAKE_BINARY_DIR}/lst" CACHE STRING "HC12 list output file directory")
ENDIF (NOT DEFINED HC12_LST_OUTPUT_DIR)

IF (NOT DEFINED HC12_ERR_OUTPUT_DIR)
	SET (HC12_ERR_OUTPUT_DIR "${CMAKE_BINARY_DIR}/err" CACHE STRING "HC12 list output file directory")
ENDIF (NOT DEFINED HC12_ERR_OUTPUT_DIR)

#
# Give the user some feedback, especially as they may not be aware these exist and have taken on defaults.
#
MESSAGE ("")
MESSAGE ("HC12 core .................: ${HC12_CORE}")
MESSAGE ("HC12 memory model .........: ${HC12_MEMORY_MODEL}")
MESSAGE ("HC12 start12*.o ...........: ${HC12_START12}")
MESSAGE ("HC12 RTL ..................: ${HC12_RTL_LIBS}")
MESSAGE ("HC12 linker prm file ......: ${HC12_LINK_PRM_FILE}")
MESSAGE ("HC12 library search dir ...: ${HC12_LIB_SEARCH_DIR}")
MESSAGE ("HC12 linker o/p directory .: ${HC12_LINK_OUTPUT_DIR}")
MESSAGE ("HC12 list file directory ..: ${HC12_LST_OUTPUT_DIR}")
MESSAGE ("HC12 error file directory .: ${HC12_ERR_OUTPUT_DIR}")
MESSAGE ("")

#
# Library and linker extensions. (Object extensions have to be specified earlier in Toolchain-Freescale-HC12.cmake)
#
SET (CMAKE_FIND_LIBRARY_PREFIXES 					"")
SET (CMAKE_FIND_LIBRARY_SUFFIXES 					".lib")
SET (CMAKE_STATIC_LIBRARY_PREFIX 					"")
SET (CMAKE_STATIC_LIBRARY_SUFFIX 					".lib")
SET (CMAKE_LINK_LIBRARY_SUFFIX						".lib")
SET (CMAKE_EXECUTABLE_SUFFIX 						".elf")

#
# C compiler.
#
SET (CMAKE_C_FLAGS_INIT 							"${CMAKE_C_FLAGS_INIT} -ViewHidden -Cc -NoEnv -NoBeep -WOutFileOn -WmsgNu=abcde")
SET (CMAKE_C_FLAGS_DEBUG_INIT 						"")
SET (CMAKE_C_FLAGS_MINSIZEREL_INIT 					"")
SET (CMAKE_C_FLAGS_RELEASE_INIT 					"")
SET (CMAKE_C_FLAGS_RELWITHDEBINFO_INIT 				"")
SET (CMAKE_C_STANDARD_LIBRARIES_INIT   				"")

#
# C++ compiler
#
SET (CMAKE_CXX_FLAGS_INIT 							"-ViewHidden -Cc -NoEnv -NoBeep -WOutFileOn -WmsgNu=abcde")
SET (CMAKE_CXX_FLAGS_DEBUG_INIT 					"")
SET (CMAKE_CXX_FLAGS_MINSIZEREL_INIT 				"")
SET (CMAKE_CXX_FLAGS_RELEASE_INIT 					"")
SET (CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT			"")
SET (CMAKE_CXX_STANDARD_LIBRARIES_INIT 				"")

#
# Flags used by the linker.
#
SET (CMAKE_EXE_LINKER_FLAGS_INIT 					"-ViewHidden -NoBeep -WErrFileOn -WmsgNu=abe -EnvERRORFILE=${HC12_LINK_OUTPUT_DIR}/link.err -EnvTEXTPATH=${HC12_LINK_OUTPUT_DIR} -L${HC12_LIB_SEARCH_DIR}")
SET (CMAKE_EXE_LINKER_FLAGS_DEBUG_INIT				"")
SET (CMAKE_EXE_LINKER_FLAGS_MINSIZEREL_INIT 		"")
SET (CMAKE_EXE_LINKER_FLAGS_RELEASE_INIT 			"")
SET (CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO			"")

SET (CMAKE_MODULE_LINKER_FLAGS_INIT 				"")
SET (CMAKE_MODULE_LINKER_FLAGS_DEBUG_INIT 			"")
SET (CMAKE_MODULE_LINKER_FLAGS_MINSIZEREL_INIT		"")
SET (CMAKE_MODULE_LINKER_FLAGS_RELEASE_INIT 		"")
SET (CMAKE_MODULE_LINKER_FLAGS_RELWITHDEBINFO_INIT	"")

SET (CMAKE_SHARED_LINKER_FLAGS_INIT 				"")
SET (CMAKE_SHARED_LINKER_FLAGS_DEBUG_INIT 			"")
SET (CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL_INIT 		"")
SET (CMAKE_SHARED_LINKER_FLAGS_RELEASE_INIT 		"")
SET (CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO_INIT	"")

#
# Compiler invocation.
#
SET (CMAKE_C_COMPILE_OBJECT 			"${HC12_PIPER} ${CMAKE_C_COMPILER} <FLAGS> -ObjN=<OBJECT> -EnvERRORFILE=${HC12_ERR_OUTPUT_DIR}/%n.err -Lasm=${HC12_LST_OUTPUT_DIR}/%n.lst <SOURCE>")
SET (CMAKE_CXX_COMPILE_OBJECT 			"${HC12_PIPER} ${CMAKE_CXX_COMPILER} <FLAGS> -ObjN=<OBJECT> -EnvERRORFILE=${HC12_ERR_OUTPUT_DIR}/%n.err -Lasm=${HC12_LST_OUTPUT_DIR}/%n.lst <SOURCE>")

#
# Librarian invocation.
#
SET (CMAKE_C_CREATE_STATIC_LIBRARY 		"${HC12_PIPER} ${CMAKE_AR} -ViewHidden -WmsgNu=abcde -NoBeep -WOutFileOn -EnvERRORFILE=${HC12_ERR_OUTPUT_DIR}/%n_lib.err -Mar{<TARGET> <OBJECTS>}")
SET (CMAKE_CXX_CREATE_STATIC_LIBRARY 	"${CMAKE_C_CREATE_STATIC_LIBRARY}")

#
# Link support.
#
# Note that we can't use CMAKE_LIBRARY_PATH_FLAG to specify the "-L" library path, as CMake will expand it
# next to the libraries which unfortunately due to the context sensitve order of the linker args will fail.
# Instead we workaround by defining -L${HC12_LIB_SEARCH_DIR} in the linker flags, but all libs need to be
# placed in this directory (or CMAKE_EXE_LINKER_FLAGS added to for other paths).
#
SET (CMAKE_LIBRARY_PATH_FLAG "")
SET (CMAKE_LINK_LIBRARY_FLAG "")

#
# Link invocation.
#
# Note we use CMake's abilitly to specify a number of separate commands to the linker
# to generate our response file, as this allows us to use GNU make (which doesn't support
# response files like NMake).
#
SET (CMAKE_C_LINK_EXECUTABLE
	"\"${CMAKE_COMMAND}\" -E echo LINK <TARGET> NAMES ${HC12_START12} <OBJECTS> <LINK_LIBRARIES> ${HC12_RTL_LIBS} END INCLUDE ${HC12_LINK_PRM_FILE} > ${CMAKE_BINARY_DIR}/link.rsp"
	"${HC12_PIPER} <CMAKE_LINKER> <LINK_FLAGS> -B ${CMAKE_BINARY_DIR}/link.rsp"
	)

#
# Create custom output directories.
#
FILE (MAKE_DIRECTORY ${HC12_LINK_OUTPUT_DIR})
FILE (MAKE_DIRECTORY ${HC12_LST_OUTPUT_DIR})
FILE (MAKE_DIRECTORY ${HC12_ERR_OUTPUT_DIR})

