/*
 * Os_Arch_Posix.c
 *
 *  Created on: 31 dec 2014
 *      Author: joakim
 */

#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>
#include "Os.h"

ucontext_t  Os_Arch_State[OS_TASK_COUNT];
int         Os_Arch_Isr;

void Os_Arch_Alarm(int signal)
{
    Os_Arch_Isr = 1;
    Os_Isr();
    Os_Arch_Isr = 0;
}

void       Os_Arch_Init(void)
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

void       Os_Arch_DisableAllInterrupts(void)
{
    sigset_t  set;
    sigemptyset(&set);
    sigaddset(&set, SIGALRM);
    sigprocmask(SIG_BLOCK, &set, NULL);
}

void       Os_Arch_EnableAllInterrupts(void)
{
    sigset_t  set;
    sigemptyset(&set);
    sigaddset(&set, SIGALRM);
    sigprocmask(SIG_UNBLOCK, &set, NULL);
}

void       Os_Arch_StoreState(Os_TaskType task)
{
    ucontext_t* ctx = &Os_Arch_State[task];
    getcontext(ctx);
}

void       Os_Arch_RestoreState(Os_TaskType task)
{
    if (Os_Arch_Isr) {
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

void       Os_Arch_PrepareState(Os_TaskType task)
{
    ucontext_t* ctx = &Os_Arch_State[task];
    getcontext(ctx);
    ctx->uc_link          = NULL;
    ctx->uc_stack.ss_size = Os_TaskConfigs[task].stack_size;
    ctx->uc_stack.ss_sp   = Os_TaskConfigs[task].stack;
    makecontext(ctx, Os_TaskConfigs[task].entry, 0);
}
