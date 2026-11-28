# ATmega2560 with avr-libc, run under QEMU's mega2560 machine (Arduino
# Mega 2560: 256 KiB flash, 8 KiB RAM, 16 MHz).
#
# Needs avr-gcc, avr-libc, qemu-system-avr and Python 3, e.g. on
# Debian/Ubuntu:
#   gcc-avr avr-libc qemu-system-misc (qemu-system-avr on newer Debian)
#
# QEMU has no semihosting for AVR, so output goes to USART0 and programs
# end with a marker line, which avr-qemu-run.py turns into the exit code.

set(CMAKE_SYSTEM_NAME      Generic)
set(CMAKE_SYSTEM_PROCESSOR avr)

set(CMAKE_C_COMPILER   avr-gcc)
set(CMAKE_CXX_COMPILER avr-g++)
set(CMAKE_ASM_COMPILER avr-gcc)

set(AVR_MCU atmega2560 CACHE STRING "AVR device passed to -mmcu")

set(CMAKE_C_FLAGS_INIT   "-mmcu=${AVR_MCU} -DF_CPU=16000000UL -ffunction-sections -fdata-sections")
set(CMAKE_ASM_FLAGS_INIT "-mmcu=${AVR_MCU}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-mmcu=${AVR_MCU} -Wl,--gc-sections")

# Count instructions instead of following host time, so the results do not
# depend on the speed of the host, at about 16 million per second.
set(CMAKE_CROSSCOMPILING_EMULATOR ${CMAKE_CURRENT_LIST_DIR}/avr-qemu-run.py --icount 6)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
