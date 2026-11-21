# RV32IMAC bare metal with picolibc, run under QEMU's virt machine.
#
# Needs a riscv64-unknown-elf GCC with rv32 multilibs, picolibc and
# qemu-system-riscv32, e.g. on Debian/Ubuntu:
#   gcc-riscv64-unknown-elf picolibc-riscv64-unknown-elf qemu-system-riscv

set(CMAKE_SYSTEM_NAME      Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv32)

set(CMAKE_C_COMPILER   riscv64-unknown-elf-gcc)
set(CMAKE_CXX_COMPILER riscv64-unknown-elf-g++)
set(CMAKE_ASM_COMPILER riscv64-unknown-elf-gcc)

set(CMAKE_C_FLAGS_INIT   "-march=rv32imac_zicsr -mabi=ilp32 --specs=picolibc.specs")
set(CMAKE_ASM_FLAGS_INIT "-march=rv32imac_zicsr -mabi=ilp32")

# QEMU virt places RAM at 0x80000000 and starts there with -bios none.
# Output and the exit code go through semihosting.
set(CMAKE_EXE_LINKER_FLAGS_INIT
    "--oslib=semihost --crt0=semihost \
-Wl,--defsym=__flash=0x80000000 -Wl,--defsym=__flash_size=0x100000 \
-Wl,--defsym=__ram=0x80100000 -Wl,--defsym=__ram_size=0x100000 \
-Wl,--defsym=__stack_size=0x1000")

set(CMAKE_CROSSCOMPILING_EMULATOR
    qemu-system-riscv32 -M virt -bios none -nographic -monitor none
    -semihosting-config enable=on,target=native -kernel)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
