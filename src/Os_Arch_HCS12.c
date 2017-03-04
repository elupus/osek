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

#include "Os.h"
#include <string.h>

typedef struct Os_Arch_StateType {
    uint16_t sp;
 } Os_Arch_StateType;

Os_Arch_StateType  Os_Arch_State[OS_TASK_COUNT];
Os_Arch_StateType  Os_Arch_State_None;

Os_Arch_StateType* Os_Arch_Ctx_Prev = NULL;
Os_Arch_StateType* Os_Arch_Ctx_Next = NULL;

void Os_Arch_Init(void)
{
    memset(&Os_Arch_State     , 0, sizeof(Os_Arch_State));
    memset(&Os_Arch_State_None, 0, sizeof(Os_Arch_State_None));
    Os_Arch_Ctx_Prev = &Os_Arch_State_None;
    Os_Arch_Ctx_Next = &Os_Arch_State_None;
}

#define PPAGE_ADDRESS "0x15"

#ifdef __HIWARE__
#define OS_ARCH_STORE()                                 \
    __asm  (                                            \
            "movb " PPAGE_ADDRESS ", 0x1 , -SP\n"       \
            "ldx Os_Arch_Ctx_Prev\n"                    \
            "sts           0x0 , X\n"                   \
     )

#define OS_ARCH_RESTORE()                               \
    __asm (                                             \
            "ldx Os_Arch_Ctx_Next\n"                    \
            "lds  0x0 , X\n"                            \
            "movb 0x1 , SP+, " PPAGE_ADDRESS "\n"       \
            "movw Os_Arch_Ctx_Next, Os_Arch_Ctx_Prev\n" \
    )

#else
#define OS_ARCH_STORE()                                 \
    __asm __volatile__ (                                \
            "movw _.frame, 2   , -SP\n"                 \
            "movw _.d1   , 2   , -SP\n"                 \
            "movb " PPAGE_ADDRESS ", 0x1 , -SP\n"       \
            "ldx Os_Arch_Ctx_Prev\n"                    \
            "sts           0x0 , X\n"                   \
     )

#define OS_ARCH_RESTORE()                               \
    __asm __volatile__ (                                \
            "ldx Os_Arch_Ctx_Next\n"                    \
            "lds  0x0 , X\n"                            \
            "movb 0x1, SP+, " PPAGE_ADDRESS "\n"        \
            "movw 0x2, SP+, _.d1\n"                     \
            "movw 0x2, SP+, _.frame\n"                  \
            "movw Os_Arch_Ctx_Next, Os_Arch_Ctx_Prev"   \
    )
#endif

#pragma CODE_SEG __NEAR_SEG NON_BANKED

__interrupt __near void Os_Arch_Swi(void)
{
    OS_ARCH_STORE();
    OS_ARCH_RESTORE();
}

#pragma CODE_SEG DEFAULT

#pragma CODE_SEG __NEAR_SEG NON_BANKED

__interrupt __near void Os_Arch_Isr(void)
{
    OS_ARCH_STORE();
    Os_Isr();
    OS_ARCH_RESTORE();
}

#pragma CODE_SEG DEFAULT

void Os_Arch_SwapState   (Os_TaskType task, Os_TaskType prev)
{
    if (task == OS_INVALID_TASK) {
        Os_Arch_Ctx_Next = &Os_Arch_State_None;
    } else {
        Os_Arch_Ctx_Next = &Os_Arch_State[task];
    }

    if (Os_CallContext == OS_CONTEXT_TASK) {
        if (Os_Arch_Ctx_Next != Os_Arch_Ctx_Prev) {
            __asm ("swi\n");
        }
    }
}

void Os_Arch_Syscall_Enter(Os_SyscallStateType* state)
{
    Os_Arch_SuspendInterrupts(&state->irq);
    Os_GetTaskId(&state->task);
}

void Os_Arch_Syscall_Leave(const Os_SyscallStateType* state)
{
    Os_TaskType task;
    Os_GetTaskId(&task);
    Os_Arch_SwapState(task, state->task);
    Os_Arch_ResumeInterrupts(&state->irq);
}

void Os_Arch_PrepareState(Os_TaskType task)
{
    Os_Arch_StateType *regs = &Os_Arch_State[task];
    const Os_TaskConfigType *cfg  = &Os_TaskConfigs[task];
    uint8_t   i;

    uint8_t* ptr = (uint8_t*)((uint16_t)cfg->stack + cfg->stack_size);
    ptr -= 9;
    ptr[0] = 0x0u;       /* CCR */
    ptr[1] = 0x0u;       /* B */
    ptr[2] = 0x0u;       /* A */
    ptr[3] = 0x0u;       /* X high */
    ptr[4] = 0x0u;       /* X low  */
    ptr[5] = 0x0u;       /* Y high */
    ptr[6] = 0x0u;       /* Y low  */
    ptr[7] = ((uint32_t)cfg->entry >> 16u) & 0xffu;
    ptr[8] = ((uint32_t)cfg->entry >>  8u) & 0xffu;

    ptr -= 1;
    ptr[0] = ((uint32_t)cfg->entry >>  0u) & 0xffu; /* PPAGE */

#ifdef __GNUC__
    ptr -= 5*sizeof(int16_t);
    ((int16_t*)ptr)[0] = 0x0u; /* _d1 */
    ((int16_t*)ptr)[1] = 0x0u; /* _frame */
    ((int16_t*)ptr)[2] = 0x0u; /* _xy */
    ((int16_t*)ptr)[3] = 0x0u; /* _z */
    ((int16_t*)ptr)[4] = 0x0u; /* _tmp */
#endif

    regs->sp  = (uint16_t)ptr;
}
