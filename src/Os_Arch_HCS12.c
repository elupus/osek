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

volatile uint8 Os_Arch_CRGFLG @ 0x37u;
volatile uint8 Os_Arch_CRGINT @ 0x38u;
volatile uint8 Os_Arch_RTICTL @ 0x3bu;

#define OS_ARCH_CFGFLG_RTIF_MASK 0x80u
#define OS_ARCH_CFGINT_RTIE_MASK 0x80u

typedef struct Os_Arch_StateType {
    uint16_t sp;
    Os_SyscallParamType* param;
    Os_StatusType        res;
 } Os_Arch_StateType;

Os_Arch_StateType  Os_Arch_State[OS_TASK_COUNT];
Os_Arch_StateType  Os_Arch_State_None;

Os_Arch_StateType* Os_Arch_Ctx_Prev = NULL;


static Os_Arch_StateType * Os_Arch_GetContext(void)
{
    Os_Arch_StateType * ctx;
    Os_TaskType         task;

    (void)Os_GetTaskId(&task);
    if (task == OS_INVALID_TASK) {
        ctx = &Os_Arch_State_None;
    }
    else {
        ctx = &Os_Arch_State[task];
    }
    return ctx;
}

void Os_Arch_Init(void)
{
    memset(&Os_Arch_State     , 0, sizeof(Os_Arch_State));
    memset(&Os_Arch_State_None, 0, sizeof(Os_Arch_State_None));
    Os_Arch_Ctx_Prev = &Os_Arch_State_None;

    Os_Arch_RTICTL   = OS_ARCH_RTICTL_VALUE;
    Os_Arch_CRGINT   = OS_ARCH_CFGINT_RTIE_MASK;
}

#define XSTR(x) STR(x)
#define STR(x) #x

#define PPAGE_ADDRESS XSTR(__PPAGE_ADR__)

#ifdef __HIWARE__
#define OS_ARCH_STORE()                                 \
    __asm  (                                            \
            "movb " PPAGE_ADDRESS ", 0x1 , -SP\n"       \
            "ldx Os_Arch_Ctx_Prev\n"                    \
            "sts           0x0 , X\n"                   \
     )

#define OS_ARCH_RESTORE()                               \
    __asm (                                             \
            "ldx Os_Arch_Ctx_Prev\n"                    \
            "lds  0x0 , X\n"                            \
            "movb 0x1 , SP+, " PPAGE_ADDRESS "\n"       \
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
            "ldx Os_Arch_Ctx_Prev\n"                    \
            "lds  0x0 , X\n"                            \
            "movb 0x1, SP+, " PPAGE_ADDRESS "\n"        \
            "movw 0x2, SP+, _.d1\n"                     \
            "movw 0x2, SP+, _.frame\n"                  \
    )
#endif

#pragma CODE_SEG __NEAR_SEG NON_BANKED

__interrupt __near void Os_Arch_Swi(void)
{
    OS_ARCH_STORE();
    {
        Os_Arch_Ctx_Prev->res = Os_Syscall_Internal(Os_Arch_Ctx_Prev->param);
        Os_Arch_Ctx_Prev = Os_Arch_GetContext();
    }
    OS_ARCH_RESTORE();
}

#pragma CODE_SEG DEFAULT

#pragma CODE_SEG __NEAR_SEG NON_BANKED

__interrupt __near void Os_Arch_Isr(void)
{
    OS_ARCH_STORE();
    {
        Os_Isr();
        Os_Arch_Ctx_Prev = Os_Arch_GetContext();
        Os_Arch_CRGFLG = OS_ARCH_CFGFLG_RTIF_MASK;
    }
    OS_ARCH_RESTORE();
}

#pragma CODE_SEG DEFAULT

Os_StatusType Os_Arch_Syscall(Os_SyscallParamType* param)
{
    Os_Arch_Ctx_Prev->param = param;
    __asm ("swi\n");
    return Os_Arch_Ctx_Prev->res;
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

void Os_Arch_Start(void)
{
    Os_SyscallParamType param = {0};
    Os_Arch_Ctx_Prev->param = &param;
    __asm ("swi\n");
    Os_Arch_EnableAllInterrupts();
}
