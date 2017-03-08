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
#include <stdarg.h>
#include "Os.h"

typedef struct Os_Arch_CtxType {
    ucontext_t ctx;
    boolean    run;
} Os_Arch_CtxType;

Os_Arch_CtxType  Os_Arch_State_None;
Os_Arch_CtxType  Os_Arch_State[OS_TASK_COUNT];
Os_StatusType Os_Arch_Syscall(Os_SyscallParamType *param);

static __inline Os_Arch_CtxType * Os_Arch_GetContext(void)
{
    Os_Arch_CtxType * ctx;
    Os_TaskType  task;

    (void)Os_GetTaskId(&task);
    if (task == OS_INVALID_TASK) {
        ctx = &Os_Arch_State_None;
    } else {
        ctx = &Os_Arch_State[task];
    }
    return ctx;
}

void Os_Arch_Alarm(int signal)
{
    Os_Arch_CtxType* ctx_before;
    Os_Arch_CtxType* ctx_after;

    ctx_before = Os_Arch_GetContext();
    Os_Isr();
    ctx_after  = Os_Arch_GetContext();

    if (ctx_before->run == FALSE) {
         ctx_after->run = TRUE;
         setcontext(&ctx_after->ctx);
     } else if (ctx_before != ctx_after) {
         ctx_after->run = TRUE;
         swapcontext(&ctx_before->ctx, &ctx_after->ctx);
     } else {
         /* nop */
     }
}

void Os_Arch_Init(void)
{
    int res;
    Os_TaskType task;
    Os_Arch_DisableAllInterrupts();

    memset(&Os_Arch_State_None, 0, sizeof(Os_Arch_State_None));
    memset(&Os_Arch_State, 0, sizeof(Os_Arch_State));
    Os_Arch_State_None.run = TRUE;
    for (task = 0u; task < OS_TASK_COUNT; ++task) {
        ucontext_t* ctx = &Os_Arch_State[task].ctx;
        getcontext(ctx);
    }

    struct sigaction sact;
    memset(&sact, 0, sizeof(sact));
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
    Os_Arch_CtxType* ctx = &Os_Arch_State[task];
    sigdelset(&ctx->ctx.uc_sigmask, SIGALRM); /* we start with interrupts enabled */
    ctx->ctx.uc_link           = NULL;
    ctx->ctx.uc_stack.ss_size  = Os_TaskConfigs[task].stack_size;
    ctx->ctx.uc_stack.ss_sp    = Os_TaskConfigs[task].stack;
    ctx->ctx.uc_stack.ss_flags = 0;
    ctx->run = FALSE;
    makecontext(&ctx->ctx, Os_TaskConfigs[task].entry, 0);
}

Os_StatusType Os_Arch_Syscall(Os_SyscallParamType* param)
{
    Os_StatusType    res;
    Os_IrqState      state;
    Os_Arch_CtxType* ctx_before;
    Os_Arch_CtxType* ctx_after;
    Os_Arch_SuspendInterrupts(&state);

    ctx_before = Os_Arch_GetContext();
    res = Os_Syscall_Internal(param);
    ctx_after  = Os_Arch_GetContext();

    if (ctx_before->run == FALSE) {
        ctx_after->run = TRUE;
        setcontext(&ctx_after->ctx);
    } else if (ctx_before != ctx_after) {
        ctx_after->run = TRUE;
        swapcontext(&ctx_before->ctx, &ctx_after->ctx);
    } else {
        /* nop */
    }

    Os_Arch_ResumeInterrupts(&state);
    return res;
}
