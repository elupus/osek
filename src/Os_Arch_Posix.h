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

typedef    sigset_t Os_IrqState;

void       Os_Arch_Init(void);

void       Os_Arch_DisableAllInterrupts(void);
void       Os_Arch_EnableAllInterrupts(void);

void       Os_Arch_SuspendInterrupts(Os_IrqState* mask);
void       Os_Arch_ResumeInterrupts(const Os_IrqState* mask);

void       Os_Arch_SwapState   (Os_TaskType task, Os_TaskType prev);
void       Os_Arch_PrepareState(Os_TaskType task);

static __inline void Os_Arch_Wait(void)
{
    /* NOP */
}

#endif /* OS_ARCH_POSIX_H_ */
