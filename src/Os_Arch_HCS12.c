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

typedef struct Os_Arch_StateType {
    uint16_t sp;
    uint8_t  ccr;
    uint16_t frame;
    uint16_t tmp;
    uint16_t z;
    uint16_t xy;
    uint16_t d1;
 } Os_Arch_StateType;

Os_Arch_StateType  Os_Arch_State[OS_TASK_COUNT];
Os_Arch_StateType  Os_Arch_State_None;

Os_Arch_StateType* Os_Arch_Ctx_Prev = NULL;
Os_Arch_StateType* Os_Arch_Ctx_Next = NULL;

void Os_Arch_Init(void)
{
    memset(&Os_Arch_State, 0, sizeof(Os_Arch_State));
    memset(&Os_Arch_State_None, 0, sizeof(Os_Arch_State_None));
    Os_Arch_Ctx_Prev = &Os_Arch_State_None;
    Os_Arch_Ctx_Next = &Os_Arch_State_None;
}

static __inline void Os_Arch_Store(void)
{
    __asm ( "ldx Os_Arch_Ctx_Prev\n"
            "sts           0x0 , X\n"
            "tpa\n"
            "staa          0x2 , X\n"
            "movw _.frame, 0x3 , X\n"
            "movw _.tmp  , 0x5 , X\n"
            "movw _.z    , 0x7 , X\n"
            "movw _.xy   , 0x9 , X\n"
            /* "movw _.d1   , 0x11, X\n" */
    ::: "x", "a");
}

static __inline void Os_Arch_Restore(void)
{
    __asm ( "ldx Os_Arch_Ctx_Next\n"
            "lds  0x0 , X\n"
            "ldaa 0x2 , X\n"
            "tap\n"
            "movw 0x3 , X, _.frame\n"
            "movw 0x5 , X, _.tmp\n"
            "movw 0x7 , X, _.z\n"
            "movw 0x9 , X, _.xy\n"
            /* "movw 0x11, X, _.d1\n" */
            "movw Os_Arch_Ctx_Next, Os_Arch_Ctx_Prev\n"
    ::: "x", "a");
}

__attribute__((interrupt)) void Os_Arch_Swi(void)
{
    Os_Arch_Store();
    Os_Arch_Restore();
}

__attribute__((interrupt)) void Os_Arch_Isr(void)
{
    Os_Arch_Store();
    Os_Isr();
    if (Os_Arch_Ctx_Next != Os_Arch_Ctx_Prev) {
        Os_Arch_Restore();
    }
}

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

void Os_Arch_PrepareState(Os_TaskType task)
{
    Os_Arch_StateType *regs = &Os_Arch_State[task];
    Os_TaskConfigType *cfg  = &Os_TaskConfigs[task];
    uint8_t   i;
    uint16_t* ptr = (uint16_t*)((uint16_t)cfg->stack + cfg->stack_size);

    ptr -= 4;
    ptr[0] = cfg->entry; /* PC */
    ptr[1] = 0x1918u;    /* D */
    ptr[2] = 0x1716u;    /* X */
    ptr[3] = 0x1514u;    /* Y */

    regs->sp  = (uint16_t)ptr;
    regs->ccr = 0x13u;
}
