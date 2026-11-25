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
 * @brief ARM Cortex-M3 port (ARMv7-M without floating point state)
 *
 * System calls are direct calls: they run the kernel on the caller's own
 * stack with the kernel's interrupt level masked through BASEPRI, and pend
 * PendSV when another task, or a fresh start of the caller, should run.
 * PendSV does every task switch, on the main stack, while tasks run in
 * thread mode on the process stack. The tick is SysTick.
 *
 * Interrupt priorities, as written to the priority registers (lower values
 * are more urgent, the core implements only the upper bits):
 * - 0x00 up to below OS_ARCH_KERNEL_PRIORITY: interrupts above the kernel,
 *   never masked by it, like OSEK category 1 ISRs. They must not call any
 *   service.
 * - OS_ARCH_KERNEL_PRIORITY: SysTick. Services and the switch code set
 *   BASEPRI to this level, which masks SysTick and everything below it.
 * - 0xFF, the lowest level: PendSV, so a switch only happens once no
 *   other handler is active.
 *
 * DisableAllInterrupts uses PRIMASK and masks everything, as OSEK defines.
 *
 * The code calling Os_Start becomes the idle context: Os_Arch_Start moves
 * it to the process stack, keeping the stack memory it already uses, and
 * gives handlers a stack of their own. Every context, the idle one
 * included, is then saved and resumed the same way.
 */

#ifndef OS_ARCH_CORTEXM_H_
#define OS_ARCH_CORTEXM_H_

#include "Std_Types.h"

typedef    uint32_t Os_IrqState;

/** bytes a context switch leaves on a task stack: the exception frame of
 * r0-r3, r12, lr, pc and xPSR, the alignment word the core may add to it,
 * and r4-r11 saved by PendSV */
#define OS_ARCH_FRAME_BYTES ((8u + 1u + 8u) * 4u)

/** stack the kernel uses below a service call, 128 bytes at most with GCC
 * 14 at -Os and -O2 and 224 at -O0, from -fcallgraph-info=su. The hooks it
 * calls are application code and count in the task's own usage */
#ifndef OS_ARCH_SYSCALL_STACK
#ifdef __OPTIMIZE__
#define OS_ARCH_SYSCALL_STACK 128u
#else
#define OS_ARCH_SYSCALL_STACK 256u
#endif
#endif

/** services run on the caller's stack, and a switch saves the context on
 * it, rounded up to keep stacks 8 byte aligned */
#define OS_ARCH_STACK_EXTRA ((OS_ARCH_FRAME_BYTES + OS_ARCH_SYSCALL_STACK + 7u) & ~7u)

#ifndef OS_ARCH_CPU_HZ
#define OS_ARCH_CPU_HZ            25000000u  /**< core clock SysTick counts, 25 MHz on QEMU mps2-an385 */
#endif

#ifndef OS_ARCH_KERNEL_PRIORITY
#define OS_ARCH_KERNEL_PRIORITY   0x80u      /**< priority of SysTick and BASEPRI of the kernel, see above */
#endif

#ifndef OS_ARCH_HANDLER_STACK_SIZE
#define OS_ARCH_HANDLER_STACK_SIZE 1024u     /**< main stack all handlers run on once started */
#endif

#if (OS_ARCH_KERNEL_PRIORITY == 0u) || (OS_ARCH_KERNEL_PRIORITY > 0xC0u)
#error "OS_ARCH_KERNEL_PRIORITY must leave a lower level for PendSV, with 3 implemented priority bits"
#endif

void       Os_Arch_Init(void);

static __inline void Os_Arch_DisableAllInterrupts(void)
{
    __asm__ __volatile__ ("cpsid i" ::: "memory");
}

static __inline void Os_Arch_EnableAllInterrupts(void)
{
    __asm__ __volatile__ ("cpsie i" ::: "memory");
}

static __inline void Os_Arch_SuspendInterrupts(Os_IrqState* mask)
{
    Os_IrqState prev;
    /* basepri_max only ever raises the level, so this nests */
    __asm__ __volatile__ ("mrs %0, basepri"   "\n\t"
                          "msr basepri_max, %1" "\n\t"
                          "isb"
                          : "=&r"(prev) : "r"(OS_ARCH_KERNEL_PRIORITY) : "memory");
    *mask = prev;
}

static __inline void Os_Arch_ResumeInterrupts(const Os_IrqState* mask)
{
    /* the isb lets a pended PendSV in before the next instruction */
    __asm__ __volatile__ ("msr basepri, %0" "\n\t"
                          "isb"
                          :: "r"(*mask) : "memory");
}

void       Os_Arch_PrepareState(Os_TaskType task);

void       Os_Arch_Start(void);

static __inline void Os_Arch_Wait(void)
{
    /* sleep until the next interrupt, normally the tick */
    __asm__ __volatile__ ("wfi" ::: "memory");
}

/** handlers the vector table must point to */
void       Os_Arch_PendSVHandler(void);
void       Os_Arch_SysTickHandler(void);

#endif /* OS_ARCH_CORTEXM_H_ */
