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

typedef void(*Os_Arch_IsrType)(void);

LPVOID             Os_Arch_System;
HANDLE             Os_Arch_Timer;
LONG               Os_Arch_Interrupt_Mask;
Os_Arch_IsrType    Os_Arch_Interrupt_Pending;
Os_Arch_StateType  Os_Arch_State[OS_TASK_COUNT];
HANDLE             Os_Arch_Thread;

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

static LPVOID Os_Arch_GetCtx(void)
{
    Os_TaskType   task;
    LPVOID        res;

    Os_GetTaskId(&task);

    if (task == OS_INVALID_TASK) {
        res = Os_Arch_System;
    } else {
        res = Os_Arch_State[task].fiber;
    }
    return res;
}

void Os_Arch_Isr(Os_Arch_IsrType isr)
{
    Os_IrqState   state;
    LPVOID        ctx_before;
    LPVOID        ctx_after;

    Os_Arch_SuspendInterrupts(&state);

    ctx_before = Os_Arch_GetCtx();

    isr();

    ctx_after  = Os_Arch_GetCtx();

    if (ctx_before != ctx_after) {
        SwitchToFiber(ctx_after);
    }

    Os_Arch_ResumeInterrupts(&state);
}

extern void Os_Arch_Trampoline(void);

void Os_Arch_InjectFunction(CONTEXT* ctx, void* fun, void *param)
{
#if defined(__x86_64) || defined(_AMD64_)
    DWORD64* stack_ptr;
    stack_ptr    = (DWORD64*)ctx->Rsp;
    stack_ptr   -= 8;
    stack_ptr[0] = ctx->Rax;
    stack_ptr[1] = ctx->Rcx;
    stack_ptr[2] = ctx->Rdx;
    stack_ptr[3] = ctx->R8;
    stack_ptr[4] = ctx->R9;
    stack_ptr[5] = ctx->R10;
    stack_ptr[6] = ctx->R11;
    stack_ptr[7] = ctx->Rip;
    ctx->Rsp = (DWORD64)stack_ptr;
    ctx->Rip = (DWORD64)Os_Arch_Trampoline;
    ctx->Rcx = (DWORD64)param;
    ctx->Rdx = (DWORD64)fun;

#elif defined(__x86) || defined(_X86_)
    DWORD* stack_ptr;
    stack_ptr    = (DWORD*)ctx->Esp;
    stack_ptr   -= 6;
    stack_ptr[0] = (DWORD)param;
    stack_ptr[1] = ctx->Eax;
    stack_ptr[2] = ctx->Ecx;
    stack_ptr[3] = ctx->Edx;
    stack_ptr[4] = ctx->Ebp;
    stack_ptr[5] = ctx->Eip;
    ctx->Ebp = (DWORD)&stack_ptr[4]; /* point to ebp pushed value */
    ctx->Esp = (DWORD)stack_ptr;
    ctx->Ecx = (DWORD)fun;
    ctx->Eip = (DWORD)Os_Arch_Trampoline;
#else
#error Not supported
#endif
}

VOID CALLBACK Os_Arch_TimerCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
    /* add to interrupt queue */
    InterlockedExchangePointer((volatile PVOID*)&Os_Arch_Interrupt_Pending, Os_Isr);

    /* see if we need to inject */
    SuspendThread(Os_Arch_Thread);
    if (Os_Arch_Interrupt_Mask == 0u) {

        /* pop queued interrupts */
        Os_Arch_IsrType isr = InterlockedExchangePointer((volatile PVOID*)&Os_Arch_Interrupt_Pending, NULL);
        if (isr) {
            CONTEXT ctx;
            memset(&ctx, 0, sizeof(ctx));
            ctx.ContextFlags = CONTEXT_ALL;

            if (GetThreadContext(Os_Arch_Thread, &ctx)) {
                Os_Arch_InjectFunction(&ctx, Os_Arch_Isr, isr);
                SetThreadContext(Os_Arch_Thread, &ctx);
            }
            else {
                exit(-1);
            }
        }
    }
    ResumeThread(Os_Arch_Thread);
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
    }
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
    *mask = InterlockedExchange(&Os_Arch_Interrupt_Mask, 0x1);
}

void Os_Arch_ResumeInterrupts(const Os_IrqState* mask)
{
    InterlockedExchange(&Os_Arch_Interrupt_Mask, *mask);
    if (*mask == 0u) {
        /* interrupts now enabled, so serve any queued up interrupts first */
        Os_Arch_IsrType isr = InterlockedExchangePointer((volatile PVOID*)&Os_Arch_Interrupt_Pending, NULL);
        if (isr) {
            Os_Arch_Isr(isr);
        }
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

Os_StatusType Os_Arch_Syscall(Os_SyscallParamType* param)
{
    Os_IrqState   state;
    LPVOID        ctx_before;
    LPVOID        ctx_after;
    Os_StatusType res;

    Os_Arch_SuspendInterrupts(&state);

    ctx_before = Os_Arch_GetCtx();

    res = Os_Syscall_Internal(param);

    ctx_after = Os_Arch_GetCtx();

    if (ctx_before != ctx_after) {
        SwitchToFiber(ctx_after);
    }

    Os_Arch_ResumeInterrupts(&state);

    return res;
}

void Os_Arch_Start(void)
{
    LPVOID ctx = Os_Arch_GetCtx();
    if (ctx != Os_Arch_System) {
        SwitchToFiber(Os_Arch_GetCtx());
    }
    Os_Arch_EnableAllInterrupts();
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
