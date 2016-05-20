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

#include <Windows.h>
#include <WinBase.h>
#include <signal.h>
#include <assert.h>

#include "Os.h"
#include "Os_Arch_Fibers.h"

typedef struct Os_Arch_StateType {
    LPVOID fiber_old;
    LPVOID fiber;
} Os_Arch_StateType;

LPVOID            Os_Arch_System;
HANDLE            Os_Arch_Timer;
CRITICAL_SECTION  Os_Arch_Section;
uint32            Os_Arch_Section_Count;
Os_Arch_StateType Os_Arch_State[OS_TASK_COUNT];
HANDLE            Os_Arch_Thread;

extern void Os_Arch_Trap(void);

static void Os_Arch_DeleteFiber(PVOID* fiber)
{
    if(*fiber && (*fiber != GetCurrentFiber())) {
        DeleteFiber(*fiber);
        *fiber = NULL;
    }
}

VOID CALLBACK Os_Arch_FiberStart(LPVOID lpParameter)
{
    Os_TaskType task = (Os_TaskType)(intptr_t)lpParameter;
    Os_Arch_DeleteFiber(&Os_Arch_State[task].fiber_old);
    Os_Arch_EnableAllInterrupts();
    Os_TaskConfigs[task].entry();
}

void Os_Arch_Isr(void (*isr)())
{
    Os_TaskType task_before, task_after;
    Os_Arch_DisableAllInterrupts();

    (void)Os_GetTaskId(&task_before);
    isr();
    (void)Os_GetTaskId(&task_after);

    if (task_before != task_after) {
        assert(Os_Arch_Section_Count != 0);
        if (task_after == OS_INVALID_TASK) {
            SwitchToFiber(Os_Arch_System);
        } else {
            SwitchToFiber(Os_Arch_State[task_after].fiber);
        }
    }

    Os_Arch_EnableAllInterrupts();
}

void Os_Arch_InjectFunction(void* fun, void (*isr)())
{
    EnterCriticalSection(&Os_Arch_Section);
    SuspendThread(Os_Arch_Thread);

    CONTEXT ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.ContextFlags = CONTEXT_ALL;

    if (GetThreadContext(Os_Arch_Thread, &ctx)) {
#if defined(__x86_64)
    	DWORD64* stack_ptr;
        stack_ptr    = (DWORD64*)ctx.Rsp;
        stack_ptr   -= 8;
        stack_ptr[0] = ctx.Rax;
        stack_ptr[1] = ctx.Rcx;
        stack_ptr[2] = ctx.Rdx;
        stack_ptr[3] = ctx.R8;
        stack_ptr[4] = ctx.R9;
        stack_ptr[5] = ctx.R10;
        stack_ptr[6] = ctx.R11;
        stack_ptr[7] = ctx.Rip;
        ctx.Rsp = (DWORD64)stack_ptr;
        ctx.Rip = (DWORD64)Os_Arch_Trap;
        ctx.Rcx = (DWORD64)isr;

#elif defined(__x86) || defined(_X86_)
        DWORD* stack_ptr;
        stack_ptr    = (DWORD*)ctx.Esp;
        stack_ptr   -= 3;
        stack_ptr[0] = (DWORD)isr;
        stack_ptr[1] = ctx.Ebp;
        stack_ptr[2] = ctx.Eip;
        ctx.Ebp = ctx.Esp;
        ctx.Esp = (DWORD)stack_ptr;
        ctx.Eip = (DWORD)Os_Arch_Trap;
#else
#error Not supported
#endif
        SetThreadContext(Os_Arch_Thread, &ctx);

    } else {
        exit(-1);
    }
    ResumeThread(Os_Arch_Thread);
    LeaveCriticalSection(&Os_Arch_Section);
}

VOID CALLBACK Os_Arch_TimerCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
    Os_Arch_InjectFunction(Os_Arch_Isr, Os_Isr);
}

void Os_Arch_Deinit(void)
{
    if (Os_Arch_Thread) {
        CloseHandle(Os_Arch_Thread);
        Os_Arch_Thread = NULL;
    }

    if (Os_Arch_Timer) {
        DeleteTimerQueueTimer( NULL
                             , Os_Arch_Timer
                             , INVALID_HANDLE_VALUE );
        Os_Arch_Timer = NULL;
    }

    Os_TaskType task;
    for (task = 0; task < OS_TASK_COUNT; ++task) {
        Os_Arch_DeleteFiber(&Os_Arch_State[task].fiber_old);
        Os_Arch_DeleteFiber(&Os_Arch_State[task].fiber);
    }

    if (Os_Arch_System) {
        ConvertFiberToThread();
        Os_Arch_System = NULL;
        DeleteCriticalSection(&Os_Arch_Section);
    }
    Os_Arch_Section_Count = 0;
}

void Os_Arch_Init(void)
{
    Os_Arch_Deinit();

    memset(&Os_Arch_State, 0, sizeof(Os_Arch_State));

    DuplicateHandle(GetCurrentProcess()
                  , GetCurrentThread()
                  , GetCurrentProcess()
                  , &Os_Arch_Thread
                  , 0
                  , FALSE
                  , DUPLICATE_SAME_ACCESS);

    Os_Arch_System = ConvertThreadToFiber(NULL);
    if (Os_Arch_System == NULL) {
        exit(GetLastError());
    }

    Os_Arch_Section_Count = 0;
    InitializeCriticalSection(&Os_Arch_Section);

    Os_Arch_DisableAllInterrupts();

    CreateTimerQueueTimer( &Os_Arch_Timer
                         , NULL
                         , Os_Arch_TimerCallback
                         , NULL
                         , OS_TICK_US / 1000
                         , OS_TICK_US / 1000
                         , WT_EXECUTEINTIMERTHREAD);
}

void Os_Arch_SuspendInterrupts(Os_IrqState* mask)
{
    EnterCriticalSection(&Os_Arch_Section);
    *mask = Os_Arch_Section_Count;
    Os_Arch_Section_Count++;
    if(Os_Arch_Section_Count != 1) {
        Os_Arch_Section_Count = 1;
        LeaveCriticalSection(&Os_Arch_Section);
    }
}

void Os_Arch_ResumeInterrupts(const Os_IrqState* mask)
{
    Os_Arch_Section_Count = *mask;
    if(*mask == 0u) {
        LeaveCriticalSection(&Os_Arch_Section);
    }
}

void Os_Arch_DisableAllInterrupts(void)
{
    Os_IrqState state;
    Os_Arch_SuspendInterrupts(&state);
}

void Os_Arch_EnableAllInterrupts(void)
{
    Os_IrqState state = 0u;
    Os_Arch_ResumeInterrupts(&state);
}

void Os_Arch_SwapState(Os_TaskType task, Os_TaskType prev)
{
    assert(Os_Arch_Section_Count != 0);
    if (Os_CallContext == OS_CONTEXT_TASK) {
        if (task == OS_INVALID_TASK) {
            SwitchToFiber(Os_Arch_System);
        } else {
            SwitchToFiber(Os_Arch_State[task].fiber);
        }
    } else {
        /* NOP - switch will occur at end of ISR */
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
