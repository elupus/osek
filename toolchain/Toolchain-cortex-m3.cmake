# ARM Cortex-M3 bare metal with picolibc, run under QEMU's mps2-an385 machine.
#
# Needs arm-none-eabi-gcc, picolibc and qemu-system-arm, e.g. on
# Debian/Ubuntu:
#   gcc-arm-none-eabi picolibc-arm-none-eabi qemu-system-arm

set(CMAKE_SYSTEM_NAME      Generic)
set(CMAKE_SYSTEM_PROCESSOR cortex-m3)

set(CMAKE_C_COMPILER   arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)

set(CMAKE_C_FLAGS_INIT   "-mcpu=cortex-m3 -mthumb --specs=picolibc.specs")
set(CMAKE_ASM_FLAGS_INIT "-mcpu=cortex-m3 -mthumb")

# mps2-an385 has 4 MiB of SSRAM at 0, where the core reads the vector table
# at reset, used as flash here, and 4 MiB at 0x20000000. Output and the exit
# code go through semihosting.
set(CMAKE_EXE_LINKER_FLAGS_INIT
    "--oslib=semihost --crt0=semihost \
-Wl,--defsym=__flash=0x00000000 -Wl,--defsym=__flash_size=0x400000 \
-Wl,--defsym=__ram=0x20000000 -Wl,--defsym=__ram_size=0x400000 \
-Wl,--defsym=__stack_size=0x1000")

# icount runs one instruction per nanosecond of virtual time, so the tick
# falls at the same instruction on every run and the counts repeat.
set(CMAKE_CROSSCOMPILING_EMULATOR
    qemu-system-arm -M mps2-an385 -nographic -monitor none
    -icount shift=0,sleep=off
    -semihosting-config enable=on,target=native -kernel)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
