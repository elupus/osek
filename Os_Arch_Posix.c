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
#include "Os.h"

ucontext_t  Os_Arch_State[OS_TASK_COUNT];

void Os_Arch_Alarm(int signal)
{
    Os_Isr();
}

void Os_Arch_Init(void)
{
    Os_Arch_DisableAllInterrupts();
    int res;

    struct sigaction sact;
    sigemptyset( &sact.sa_mask );
    sact.sa_flags   = SA_RESTART;
    sact.sa_handler = Os_Arch_Alarm;
    res = sigaction(SIGALRM, &sact, NULL);

     // start up the "interrupt"!
    struct itimerval val;
    val.it_interval.tv_sec = val.it_value.tv_sec = 0;
    val.it_interval.tv_usec = val.it_value.tv_usec = 100000;
    res = setitimer(ITIMER_REAL, &val, NULL);
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

void Os_Arch_StoreState(Os_TaskType task)
{
    ucontext_t* ctx = &Os_Arch_State[task];
    getcontext(ctx);
}

static void Os_Arch_RestoreState(Os_TaskType task)
{
    if (Os_CallContext == OS_CONTEXT_ISR) {
        ucontext_t ctx;
        getcontext(&ctx);
        if (ctx.uc_link != &Os_Arch_State[task]) {
            ctx.uc_link = &Os_Arch_State[task];
            setcontext(&ctx);
        }
    } else {
        ucontext_t* ctx = &Os_Arch_State[task];
        setcontext(ctx);
    }
}

void Os_Arch_PrepareState(Os_TaskType task)
{
    ucontext_t* ctx = &Os_Arch_State[task];
    getcontext(ctx);
    ctx->uc_link          = NULL;
    ctx->uc_stack.ss_size = Os_TaskConfigs[task].stack_size;
    ctx->uc_stack.ss_sp   = Os_TaskConfigs[task].stack;
    makecontext(ctx, Os_TaskConfigs[task].entry, 0);
}

void Os_Arch_SwapState(Os_TaskType task, Os_TaskType prev)
{
    if (prev != Os_TaskIdNone) {
        Os_Arch_StoreState(prev);
    }

    if (prev != Os_TaskRunning) {
        Os_Arch_RestoreState(task);
    }
}
