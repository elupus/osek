/* OSEKOS Implementation of an OSEK Scheduler
 * Copyright (C) 2015 Joakim Plate
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * @file
 * @brief RISC-V machine mode port (RV32/RV64 without floating point state)
 *
 * The tick comes from the machine timer of a CLINT compatible timer, and
 * system calls enter the kernel with ecall. Both go through one trap entry,
 * which saves the interrupted context on the task's own stack and runs the
 * kernel on a separate kernel stack, so a task stack only needs room for
 * one saved context on top of the task's own usage.
 */

#ifndef OS_ARCH_RISCV_H_
#define OS_ARCH_RISCV_H_

#include "Std_Types.h"

typedef    uintptr_t Os_IrqState;

/** words in a saved context, see Os_Arch_RiscV_Asm.S */
#define OS_ARCH_FRAME_WORDS 32u

/** each task stack holds one saved context, the kernel runs on its own stack */
#define OS_ARCH_STACK_EXTRA (OS_ARCH_FRAME_WORDS * sizeof(uintptr_t))

#ifndef OS_ARCH_CLINT_BASE
#define OS_ARCH_CLINT_BASE      0x02000000u  /**< CLINT base address, as on QEMU virt and SiFive parts */
#endif

#ifndef OS_ARCH_MTIME_HZ
#define OS_ARCH_MTIME_HZ        10000000u    /**< machine timer frequency, 10 MHz on QEMU virt */
#endif

#ifndef OS_ARCH_KERNEL_STACK_SIZE
#define OS_ARCH_KERNEL_STACK_SIZE 2048u      /**< stack the kernel and the tick run on */
#endif

void       Os_Arch_Init(void);

#define OS_ARCH_MSTATUS_MIE 0x8u

static __inline void Os_Arch_DisableAllInterrupts(void)
{
    __asm__ __volatile__ ("csrc mstatus, %0" :: "r"(OS_ARCH_MSTATUS_MIE) : "memory");
}

static __inline void Os_Arch_EnableAllInterrupts(void)
{
    __asm__ __volatile__ ("csrs mstatus, %0" :: "r"(OS_ARCH_MSTATUS_MIE) : "memory");
}

static __inline void Os_Arch_SuspendInterrupts(Os_IrqState* mask)
{
    Os_IrqState prev;
    __asm__ __volatile__ ("csrrc %0, mstatus, %1" : "=r"(prev) : "r"(OS_ARCH_MSTATUS_MIE) : "memory");
    *mask = prev & OS_ARCH_MSTATUS_MIE;
}

static __inline void Os_Arch_ResumeInterrupts(const Os_IrqState* mask)
{
    if (*mask & OS_ARCH_MSTATUS_MIE) {
        Os_Arch_EnableAllInterrupts();
    } else {
        Os_Arch_DisableAllInterrupts();
    }
}

void       Os_Arch_PrepareState(Os_TaskType task);

void       Os_Arch_Start(void);

static __inline void Os_Arch_Wait(void)
{
    /* sleep until the next interrupt, normally the tick */
    __asm__ __volatile__ ("wfi" ::: "memory");
}

#endif /* OS_ARCH_RISCV_H_ */
