message ("Loaded: Generic-chc12-hc12.cmake")

set (HC12_CORE         "HCS12" CACHE STRING "HC12 core - one of 'HC12', 'HCS12' or 'HCS12X' (case sensitive!)")
set (HC12_MEMORY_MODEL "s"     CACHE STRING "HC12 memory model - one of s = (Small), l = (Large) or b = (Banked) (case sensitive!)")

if (${HC12_CORE} STREQUAL "HCS12X")
    set (HC12_CORE_LETTER     "x")
else ()
    set (HC12_CORE_LETTER     "")
endif ()

set (HC12_START12   "start12${HC12_CORE_LETTER}${HC12_MEMORY_MODEL}.o" CACHE STRING "HC12 startup object")
set (HC12_RTL_LIBS  "ansi${HC12_CORE_LETTER}${HC12_MEMORY_MODEL}.lib"  CACHE STRING "HC12 run-time libraries")

set (CMAKE_C_STANDARD_LIBRARIES_INIT "${CMAKE_C_STANDARD_LIBRARIES_INIT} -Add${HC12_RTL_LIBS} -Add${HC12_START12}")
set (CMAKE_C_FLAGS_INIT              "${CMAKE_C_FLAGS_INIT}              -EnvLIBPATH={Compiler}/lib/hc12c/include -M${HC12_MEMORY_MODEL} -Cpu${HC12_CORE} ")
set (CMAKE_EXE_LINKER_FLAGS_INIT     "${CMAKE_EXE_LINKER_FLAGS_INIT}     -EnvOBJPATH={Compiler}/lib/hc12c/lib")

#
# Give the user some feedback, especially as they may not be aware these exist and have taken on defaults.
#
message ("")
message ("HC12 core .................: ${HC12_CORE}")
message ("HC12 memory model .........: ${HC12_MEMORY_MODEL}")
message ("HC12 start12*.o ...........: ${HC12_START12}")
message ("HC12 RTL ..................: ${HC12_RTL_LIBS}")
message ("")

