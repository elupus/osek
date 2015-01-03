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
#include "Os_Arch_Fibers.h"

#include <Windows.h>
#include <WinBase.h>

typedef struct Os_Arch_StateType {
    LPVOID fiber_old;
    LPVOID fiber;
} Os_Arch_StateType;

LPVOID Os_Arch_System;

HANDLE Os_Arch_Timer;
CRITICAL_SECTION Os_Arch_Section;

Os_Arch_StateType Os_Arch_State[OS_TASK_COUNT];

VOID CALLBACK Os_Arch_FiberStart(LPVOID lpParameter)
{
    Os_TaskType task = (Os_TaskType)(intptr_t)lpParameter;
    if (Os_Arch_State[task].fiber_old) {
        DeleteFiber(Os_Arch_State[task].fiber_old);
        Os_Arch_State[task].fiber_old = NULL;
    }
    Os_TaskConfigs[task].entry();
}

VOID CALLBACK Os_Arch_TimerCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
    Os_Arch_DisableAllInterrupts();
    Os_Isr();
    Os_Arch_EnableAllInterrupts();
}

void Os_Arch_Init(void)
{
    memset(&Os_Arch_State, 0, sizeof(Os_Arch_State));
    Os_Arch_System = ConvertThreadToFiber(NULL);

    InitializeCriticalSection(&Os_Arch_Section);
    Os_Arch_DisableAllInterrupts();

    CreateTimerQueueTimer( &Os_Arch_Timer
                         , NULL
                         , Os_Arch_TimerCallback
                         , NULL
                         , OS_TICK_US / 1000
                         , OS_TICK_US / 1000
                         , WT_EXECUTEDEFAULT);

}

void Os_Arch_DisableAllInterrupts(void)
{
    EnterCriticalSection(&Os_Arch_Section);
}

void Os_Arch_EnableAllInterrupts(void)
{
    LeaveCriticalSection(&Os_Arch_Section);
}

void Os_Arch_SwapState(Os_TaskType task, Os_TaskType prev)
{
    SwitchToFiber(Os_Arch_State[task].fiber);
}

static __inline void Os_Arch_DeleteFiber(PVOID* fiber)
{
    if(*fiber && (*fiber != GetCurrentFiber())) {
        DeleteFiber(*fiber);
        *fiber = NULL;
    }
}

void Os_Arch_PrepareState(Os_TaskType task)
{
    /* clean any prepared fiber */
    Os_Arch_DeleteFiber(&Os_Arch_State[task].fiber_old);
    Os_Arch_DeleteFiber(&Os_Arch_State[task].fiber);

    /* queue for delete if active still exist */
    if (Os_Arch_State[task].fiber) {
        Os_Arch_State[task].fiber_old = Os_Arch_State[task].fiber;
    }

    Os_Arch_State[task].fiber = CreateFiber( Os_TaskConfigs[task].stack_size
                                           , Os_Arch_FiberStart
                                           , (LPVOID)(intptr_t)task);
}
