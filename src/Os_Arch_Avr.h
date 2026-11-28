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
 * @brief AVR port for megaAVR parts such as the ATmega2560 (avr-gcc, avr-libc)
 *
 * AVR has no trap instruction, so a system call is an ordinary call that
 * saves the registers a C caller expects to be preserved on the caller's
 * stack. The tick comes from the Timer1 compare match A interrupt, which
 * saves all registers. Both run the kernel on a separate kernel stack and
 * resume whichever context the kernel then considers running, see
 * Os_Arch_Avr_Asm.S. A task stack only needs room for one saved context
 * on top of the task's own usage.
 */

#ifndef OS_ARCH_AVR_H_
#define OS_ARCH_AVR_H_

#include "Std_Types.h"

typedef    uint8_t Os_IrqState;

/** bytes of a context saved by the tick: the return address, r0-r31,
 * SREG and RAMPZ where the part has it */
#if defined(__AVR_3_BYTE_PC__)
#define OS_ARCH_PC_BYTES 3u
#else
#define OS_ARCH_PC_BYTES 2u
#endif
#if defined(__AVR_HAVE_RAMPZ__)
#define OS_ARCH_FRAME_BYTES (OS_ARCH_PC_BYTES + 34u)
#else
#define OS_ARCH_FRAME_BYTES (OS_ARCH_PC_BYTES + 33u)
#endif

/** each task stack holds one saved context and the return address a
 * fresh task starts with, the kernel runs on its own stack */
#define OS_ARCH_STACK_EXTRA (OS_ARCH_FRAME_BYTES + OS_ARCH_PC_BYTES)

#ifndef OS_ARCH_CPU_HZ
#ifdef F_CPU
#define OS_ARCH_CPU_HZ          F_CPU
#else
#define OS_ARCH_CPU_HZ          16000000UL /**< CPU clock, 16 MHz as on the Arduino Mega 2560 */
#endif
#endif

#ifndef OS_ARCH_KERNEL_STACK_SIZE
#define OS_ARCH_KERNEL_STACK_SIZE 192u      /**< stack the kernel and the tick run on */
#endif

void       Os_Arch_Init(void);

#define OS_ARCH_SREG_I 0x80u

static __inline void Os_Arch_DisableAllInterrupts(void)
{
    __asm__ __volatile__ ("cli" ::: "memory");
}

static __inline void Os_Arch_EnableAllInterrupts(void)
{
    __asm__ __volatile__ ("sei" ::: "memory");
}

static __inline void Os_Arch_SuspendInterrupts(Os_IrqState* mask)
{
    Os_IrqState prev;
    __asm__ __volatile__ ("in %0, __SREG__" "\n\t"
                          "cli"
                          : "=r"(prev) :: "memory");
    *mask = prev & OS_ARCH_SREG_I;
}

static __inline void Os_Arch_ResumeInterrupts(const Os_IrqState* mask)
{
    if (*mask & OS_ARCH_SREG_I) {
        Os_Arch_EnableAllInterrupts();
    } else {
        Os_Arch_DisableAllInterrupts();
    }
}

void       Os_Arch_PrepareState(Os_TaskType task);

void       Os_Arch_Start(void);

static __inline void Os_Arch_Wait(void)
{
    /* sleep until the next interrupt, normally the tick, sleep is
     * enabled in idle mode by Os_Arch_Init */
    __asm__ __volatile__ ("sleep" ::: "memory");
}

#endif /* OS_ARCH_AVR_H_ */
