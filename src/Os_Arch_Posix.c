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

#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include "Os.h"

ucontext_t  Os_Arch_State_None;
boolean     Os_Arch_State_Started;
ucontext_t  Os_Arch_State[OS_TASK_COUNT];

void Os_Arch_Alarm(int signal)
{
    Os_TaskType task_before, task_after;

    (void)Os_GetTaskId(&task_before);
    Os_Isr();
    (void)Os_GetTaskId(&task_after);

    if (task_before != task_after) {
        Os_Arch_SwapState(task_after, task_before);
    }
}

void Os_Arch_Init(void)
{
    int res;
    Os_TaskType task;
    Os_Arch_DisableAllInterrupts();

    Os_Arch_State_Started = FALSE;
    memset(&Os_Arch_State_None, 0, sizeof(Os_Arch_State_None));
    memset(&Os_Arch_State, 0, sizeof(Os_Arch_State));

    for (task = 0u; task < OS_TASK_COUNT; ++task) {
        ucontext_t* ctx = &Os_Arch_State[task];
        getcontext(ctx);
    }

    struct sigaction sact;
    sigemptyset( &sact.sa_mask );
    sact.sa_flags   = SA_RESTART;
    sact.sa_handler = Os_Arch_Alarm;
    res = sigaction(SIGALRM, &sact, NULL);
    if (res == -1) {
        exit(-1);
    }

     // start up the "interrupt"!
    struct itimerval val;
    val.it_interval.tv_sec  = val.it_value.tv_sec  = OS_TICK_US / 1000000u;
    val.it_interval.tv_usec = val.it_value.tv_usec = OS_TICK_US % 1000000u;
    res = setitimer(ITIMER_REAL, &val, NULL);
    if (res == -1) {
        exit(-1);
    }
}

void Os_Arch_SuspendInterrupts(Os_IrqState* mask)
{
    sigset_t  set;
    sigemptyset(&set);
    sigaddset(&set, SIGALRM);
    sigprocmask(SIG_BLOCK, &set, mask);
}

void Os_Arch_ResumeInterrupts(const Os_IrqState* mask)
{
    sigprocmask(SIG_SETMASK, mask, NULL);
}

void Os_Arch_DisableAllInterrupts(void)
{
    sigset_t  set;
    sigemptyset(&set);
    sigaddset(&set, SIGALRM);
    sigprocmask(SIG_BLOCK, &set, NULL);
}

void Os_Arch_EnableAllInterrupts(void)
{
    sigset_t  set;
    sigemptyset(&set);
    sigaddset(&set, SIGALRM);
    sigprocmask(SIG_UNBLOCK, &set, NULL);
}

void Os_Arch_PrepareState(Os_TaskType task)
{
    ucontext_t* ctx = &Os_Arch_State[task];
    sigdelset(&ctx->uc_sigmask, SIGALRM); /* we start with interrupts enabled */
    ctx->uc_link           = NULL;
    ctx->uc_stack.ss_size  = Os_TaskConfigs[task].stack_size;
    ctx->uc_stack.ss_sp    = Os_TaskConfigs[task].stack;
    ctx->uc_stack.ss_flags = 0;
    makecontext(ctx, Os_TaskConfigs[task].entry, 0);
}

void Os_Arch_SwapState(Os_TaskType task, Os_TaskType prev)
{
    if (Os_CallContext == OS_CONTEXT_TASK) {
        ucontext_t *ctx_after, *ctx_before;
        if (task == OS_INVALID_TASK) {
            ctx_after = &Os_Arch_State_None;
        } else {
            ctx_after = &Os_Arch_State[task];
        }

        if (prev == OS_INVALID_TASK) {
            if (Os_Arch_State_Started == FALSE) {
                Os_Arch_State_Started = TRUE;
                ctx_before = &Os_Arch_State_None;
            } else {
                ctx_before = NULL;
            }
        } else {
            ctx_before = &Os_Arch_State[prev];
        }

        if (ctx_before) {
            swapcontext(ctx_before, ctx_after);
        } else {
            setcontext(ctx_after);
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
