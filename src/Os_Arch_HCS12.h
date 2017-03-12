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

#ifndef OS_ARCH_HCS12_H_
#define OS_ARCH_HCS12_H_

#include "Std_Types.h"

void       Os_Arch_Init(void);


typedef    uint8_t Os_IrqState;

typedef struct Os_SyscallStateType {
    Os_IrqState irq;
    Os_TaskType task;
} Os_SyscallStateType;

#pragma CODE_SEG __NEAR_SEG NON_BANKED
__interrupt __near void Os_Arch_Swi(void);
#pragma CODE_SEG DEFAULT

#pragma CODE_SEG __NEAR_SEG NON_BANKED
__interrupt __near void Os_Arch_Isr(void);
#pragma CODE_SEG DEFAULT

#pragma INLINE
static void __inline Os_Arch_DisableAllInterrupts(void)
{
    __asm("sei");
}


#pragma INLINE
static void __inline Os_Arch_EnableAllInterrupts(void)
{
    __asm("cli");
}

#ifdef __HIWARE__
#pragma NO_ENTRY
static void Os_Arch_SuspendInterrupts(Os_IrqState* mask)
{
    __asm (
        "tfr D,X\n"
        "tpa\n"                      /* (CCR) -> A */
        "sei\n"
        "staa 0, X\n"                /* (A)   -> (*X=mask) */
    );
}
#else
static void __inline Os_Arch_SuspendInterrupts(Os_IrqState* mask)
{
    __asm (
        "tpa\n"                      /* (CCR) -> A */
        "sei\n"
        "staa 0, X\n"                /* (A)   -> (*X=mask) */
        :: "x"(mask) : "a");
}
#endif

#ifdef __HIWARE__
#pragma NO_ENTRY
static void __inline Os_Arch_ResumeInterrupts(const Os_IrqState* mask)
{
    __asm (
        "tfr D,X\n"
        "ldaa 0, X\n"                /* (*X=mask) -> (A) */
        "tap\n"                      /* (A)       -> CCR */
    );
}
#else
static void __inline Os_Arch_ResumeInterrupts(const Os_IrqState* mask)
{
    __asm(
        "ldaa 0, X\n"                /* (*X=mask) -> (A) */
        "tap\n"                      /* (A)       -> CCR */
        :: "x"(mask) : "a");
}
#endif

void       Os_Arch_PrepareState(Os_TaskType task);

#pragma INLINE
static __inline void Os_Arch_Wait(void)
{
    /* NOP */
}

#endif /* OS_ARCH_HCS12_H_ */
