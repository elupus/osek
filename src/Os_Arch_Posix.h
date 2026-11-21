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

#ifndef OS_ARCH_POSIX_H_
#define OS_ARCH_POSIX_H_

#include <signal.h>
#include <unistd.h>

typedef    sigset_t Os_IrqState;

/* The tick is a signal delivered on the running task's stack. The kernel's
 * signal frame holds the full register state, several KB and growing with the
 * vector extensions of the CPU (AVX-512, SVE, SME), and the handler and the
 * scheduler run on top of it. */
#define OS_ARCH_STACK_EXTRA (64u * 1024u)

void       Os_Arch_Init(void);

void       Os_Arch_DisableAllInterrupts(void);
void       Os_Arch_EnableAllInterrupts(void);

void       Os_Arch_SuspendInterrupts(Os_IrqState* mask);
void       Os_Arch_ResumeInterrupts(const Os_IrqState* mask);

void       Os_Arch_PrepareState(Os_TaskType task);

void       Os_Arch_Start(void);

static __inline void Os_Arch_Wait(void)
{
    /* sleep until the next signal, normally the tick */
    pause();
}

#endif /* OS_ARCH_POSIX_H_ */
