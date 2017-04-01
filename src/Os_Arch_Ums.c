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
#include <stdio.h>

#include "Os.h"
#include "Os_Arch_Ums.h"

typedef struct Os_Arch_StateType {
    HANDLE thread;
} Os_Arch_StateType;

typedef void(*Os_Arch_IsrType)(void);

HANDLE               Os_Arch_Timer;
LONG                 Os_Arch_Interrupt_Mask;
Os_Arch_IsrType      Os_Arch_Interrupt_Pending;
Os_Arch_StateType    Os_Arch_State[OS_TASK_COUNT];
HANDLE               Os_Arch_Thread;
PUMS_CONTEXT         Os_Arch_Context;
PUMS_CONTEXT         Os_Arch_Context_Next;
PUMS_COMPLETION_LIST Os_Arch_CompletionList;

PPROC_THREAD_ATTRIBUTE_LIST Os_Arch_ThreadAttribute;


void Os_Arch_ErrorExit(LPTSTR lpszFunction, DWORD code)
{
    // Retrieve the system error message for the last-error code

    LPTSTR lpMsgBuf;

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf,
        0, NULL);

    fprintf(stderr, "%s filed with error %d: %s", lpszFunction, code, lpMsgBuf);
    exit(code);
}

DWORD WINAPI Os_Arch_Entry(LPVOID lpParameter)
{
    Os_TaskType task = (Os_TaskType)(intptr_t)lpParameter;

    Os_Arch_EnableAllInterrupts();
    Os_TaskConfigs[task].entry();
    return 0u;
}


void Os_Arch_Isr(Os_Arch_IsrType isr)
{
#if(0)
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
#endif
}

#if(0)
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
#endif

VOID CALLBACK Os_Arch_TimerCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
#if(0)
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
#endif
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
        if (Os_Arch_State[task].thread != INVALID_HANDLE_VALUE) {
            CloseHandle(Os_Arch_State[task].thread);
            Os_Arch_State[task].thread = INVALID_HANDLE_VALUE;
        }
    }


    if (Os_Arch_ThreadAttribute) {
        DeleteProcThreadAttributeList(Os_Arch_ThreadAttribute);
    }

    if (Os_Arch_CompletionList) {
        /* todo make sure it's empty */
        DeleteUmsCompletionList(Os_Arch_CompletionList);
    }

    if (Os_Arch_Context) {
        DeleteUmsThreadContext(Os_Arch_Context);
    }
}

VOID __stdcall Os_Arch_UmsSchedulerProc(
    _In_ UMS_SCHEDULER_REASON Reason,
    _In_ ULONG_PTR            ActivationPayload,
    _In_ PVOID                SchedulerParam
    )
{
    switch (Reason) {
        case UmsSchedulerStartup: {
            break;
        }

        case UmsSchedulerThreadBlocked: {
            break;
        }

        case UmsSchedulerThreadYield: {
            Os_Syscall_Internal((Os_SyscallParamType*)SchedulerParam);
            break;
        }

        default: {
            break;
        }
    }

    for (;;) {
        if (Os_Arch_Context_Next == NULL) {
            if (DequeueUmsCompletionListItems(Os_Arch_CompletionList, INFINITE, &Os_Arch_Context_Next) != TRUE) {
                exit(-1);
            }
        }

        PUMS_CONTEXT ctx = Os_Arch_Context_Next;
        Os_Arch_Context_Next = GetNextUmsListItem(ctx);

        for (;;) {
            if (ExecuteUmsThread(ctx) != TRUE) {
                if (GetLastError() == ERROR_RETRY) {
                    continue;
                } else {
                    Os_Arch_ErrorExit("Os_Arch_Init::ExecuteUmsThread", GetLastError());
                }
            }
        }
    }
}

void Os_Arch_Init(void)
{
    Os_Arch_Deinit();

    memset(&Os_Arch_State, 0, sizeof(Os_Arch_State));

    for (Os_TaskType task = 0; task < OS_TASK_COUNT; ++task) {
        Os_Arch_State[task].thread = INVALID_HANDLE_VALUE;
    }

    DuplicateHandle(GetCurrentProcess()
                  , GetCurrentThread()
                  , GetCurrentProcess()
                  , &Os_Arch_Thread
                  , 0
                  , FALSE
                  , DUPLICATE_SAME_ACCESS);


    Os_Arch_DisableAllInterrupts();

    CreateTimerQueueTimer( &Os_Arch_Timer
                         , NULL
                         , Os_Arch_TimerCallback
                         , NULL
                         , OS_TICK_US / 1000
                         , OS_TICK_US / 1000
                         , WT_EXECUTEINTIMERTHREAD);


    if (CreateUmsThreadContext(&Os_Arch_Context) != TRUE) {
        Os_Arch_ErrorExit("Os_Arch_Init::CreateUmsThreadContext", GetLastError());
    }

    if (CreateUmsCompletionList(&Os_Arch_CompletionList) != TRUE) {
        Os_Arch_ErrorExit("Os_Arch_Init::CreateUmsCompletionList", GetLastError());
    }

    Os_Arch_ThreadAttribute = NULL;
    SIZE_T      size  = 0u;
    const DWORD count = 1u;

    InitializeProcThreadAttributeList(NULL, count, 0u, &size);

    Os_Arch_ThreadAttribute = (PPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(GetProcessHeap(), 0, size);
    if (Os_Arch_ThreadAttribute == NULL) {
        Os_Arch_ErrorExit("Os_Arch_Init::HeapAlloc", GetLastError());
    }

    if (InitializeProcThreadAttributeList(Os_Arch_ThreadAttribute, count, 0u, &size) != TRUE) {
        Os_Arch_ErrorExit("Os_Arch_Init::InitializeProcThreadAttributeList", GetLastError());
    }


    UMS_CREATE_THREAD_ATTRIBUTES attributes;
    attributes.UmsVersion        = UMS_VERSION;
    attributes.UmsContext        = Os_Arch_Context;
    attributes.UmsCompletionList = Os_Arch_CompletionList;

    if (UpdateProcThreadAttribute( Os_Arch_ThreadAttribute
                                 , 0u
                                 , PROC_THREAD_ATTRIBUTE_UMS_THREAD
                                 , &attributes
                                 , sizeof(attributes)
                                 , NULL
                                 , NULL) != TRUE) {
        Os_Arch_ErrorExit("Os_Arch_Init::UpdateProcThreadAttribute", GetLastError());
    }

}

void Os_Arch_Start(void)
{
    Os_Arch_EnableAllInterrupts();

    UMS_SCHEDULER_STARTUP_INFO info = { 0 };
    info.UmsVersion     = UMS_VERSION;
    info.SchedulerProc  = Os_Arch_UmsSchedulerProc;
    info.CompletionList = Os_Arch_CompletionList;

    if (EnterUmsSchedulingMode(&info) != TRUE) {
        Os_Arch_ErrorExit("Os_Arch_Start::EnterUmsSchedulingMode", GetLastError());
    }
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
    Os_StatusType res;

    Os_Arch_SuspendInterrupts(&state);

    UmsThreadYield(param);

    Os_Arch_ResumeInterrupts(&state);

    return res;
}

void Os_Arch_PrepareState(Os_TaskType task)
{
    /* clean any prepared fiber */
    if (Os_Arch_State[task].thread != INVALID_HANDLE_VALUE) {
        CloseHandle(Os_Arch_State[task].thread);
        Os_Arch_State[task].thread = INVALID_HANDLE_VALUE;
    }

    Os_Arch_State[task].thread = CreateRemoteThreadEx( GetCurrentProcess()
                                                     , NULL
                                                     , 0
                                                     , Os_Arch_Entry
                                                     , (LPVOID)task
                                                     , 0
                                                     , Os_Arch_ThreadAttribute
                                                     , NULL);
    if (Os_Arch_State[task].thread == NULL) {
        Os_Arch_ErrorExit("Os_Arch_PrepareState::CreateRemoteThreadEx", GetLastError());
    }
}
