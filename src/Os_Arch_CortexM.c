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

#include <stdlib.h>

#include "Os.h"

/* system control block and SysTick registers */
#define OS_ARCH_ICSR            (*(volatile uint32_t*)0xE000ED04u)
#define OS_ARCH_ICSR_PENDSVSET  (1u << 28)
#define OS_ARCH_SHPR_PENDSV     (*(volatile uint8_t*)0xE000ED22u)
#define OS_ARCH_SHPR_SYSTICK    (*(volatile uint8_t*)0xE000ED23u)
#define OS_ARCH_SYST_CSR        (*(volatile uint32_t*)0xE000E010u)
#define OS_ARCH_SYST_RVR        (*(volatile uint32_t*)0xE000E014u)
#define OS_ARCH_SYST_CVR        (*(volatile uint32_t*)0xE000E018u)
#define OS_ARCH_SYST_CSR_START  0x7u /* enable, interrupt, core clock */

#define OS_ARCH_TICK_CYCLES     ((OS_ARCH_CPU_HZ / 1000u) * OS_TICK_US / 1000u)

#if (OS_ARCH_TICK_CYCLES == 0u) || (OS_ARCH_TICK_CYCLES > 0x1000000u)
#error "OS_TICK_US does not fit the 24 bit SysTick counter"
#endif

/* exception frame slots, in words */
#define OS_ARCH_FRAME_LR        5u
#define OS_ARCH_FRAME_PC        6u
#define OS_ARCH_FRAME_XPSR      7u
#define OS_ARCH_FRAME_WORDS     8u
#define OS_ARCH_XPSR_THUMB      0x01000000u

/** marks the context slot of the code running now, which has no saved
 * context. A slot reset to NULL while running means the task restarted */
#define OS_ARCH_RUNNING         ((uint32_t*)1)

extern volatile boolean Os_Continue;

static uint64_t Os_Arch_HandlerStack[OS_ARCH_HANDLER_STACK_SIZE / sizeof(uint64_t)];

/** saved process stack pointer of each task, NULL for a task to start from
 * scratch, and of the code that called Os_Start */
static uint32_t* Os_Arch_Context[OS_TASK_COUNT];
static uint32_t* Os_Arch_ContextNone;

/** context slot of the code running now. The kernel's view can differ until
 * PendSV has run */
static uint32_t** Os_Arch_Current;

uint32_t Os_Arch_Switch(uint32_t* sp);

static __inline uint32_t** Os_Arch_GetContext(void)
{
    Os_TaskType task;
    (void)Os_GetTaskId(&task);
    if (task == OS_INVALID_TASK) {
        return &Os_Arch_ContextNone;
    }
    return &Os_Arch_Context[task];
}

/** pend a switch if the kernel wants another context, or restarted the
 * running task. Called at the kernel's interrupt level */
static __inline void Os_Arch_Dispatch(void)
{
    uint32_t** next = Os_Arch_GetContext();
    if ((next != Os_Arch_Current) || (*next == NULL)) {
        OS_ARCH_ICSR = OS_ARCH_ICSR_PENDSVSET;
        __asm__ __volatile__ ("dsb" ::: "memory");
    }
}

/* a task function returned instead of terminating */
void Os_Arch_TaskReturned(void)
{
    exit(127);
}

/**
 * @brief Switch contexts, called by PendSV on the main stack
 * @param sp process stack pointer of the interrupted context, with r4-r11
 *        saved below its exception frame
 * @return saved stack pointer of the context to resume, or for a task to
 *         start from scratch its exception frame with bit 0 set
 */
uint32_t Os_Arch_Switch(uint32_t* sp)
{
    const Os_TaskConfigType* cfg;
    Os_IrqState state;
    Os_TaskType task;
    uint32_t*   frame;
    uint32_t    res;

    Os_Arch_SuspendInterrupts(&state);

    /* a task that restarted itself has nothing left to save */
    if (*Os_Arch_Current != NULL) {
        *Os_Arch_Current = sp;
    }

    Os_Arch_Current = Os_Arch_GetContext();
    if (*Os_Arch_Current != NULL) {
        res = (uint32_t)*Os_Arch_Current;
    } else {
        /* a fresh start only needs the frame exception return pops, with
         * the entry, the Thumb state and where a returning task goes */
        (void)Os_GetTaskId(&task);
        cfg   = &Os_TaskConfigs[task];
        frame = (uint32_t*)(((uintptr_t)cfg->stack + cfg->stack_size) & ~(uintptr_t)7u) - OS_ARCH_FRAME_WORDS;
        frame[OS_ARCH_FRAME_LR]   = (uint32_t)(uintptr_t)Os_Arch_TaskReturned;
        frame[OS_ARCH_FRAME_PC]   = (uint32_t)(uintptr_t)cfg->entry & ~1u;
        frame[OS_ARCH_FRAME_XPSR] = OS_ARCH_XPSR_THUMB;
        res = (uint32_t)(uintptr_t)frame | 1u;
    }
    *Os_Arch_Current = OS_ARCH_RUNNING;

    Os_Arch_ResumeInterrupts(&state);
    return res;
}

void Os_Arch_SysTickHandler(void)
{
    if (Os_Continue) {
        Os_Isr();
        Os_Arch_Dispatch();
    } else {
        /* shut down, stop the tick so no task runs again */
        OS_ARCH_SYST_CSR = 0u;
    }
}

Os_StatusType Os_Arch_Syscall(Os_SyscallParamType* param)
{
    Os_IrqState   state;
    Os_StatusType res;

    Os_Arch_SuspendInterrupts(&state);
    res = Os_Syscall_Internal(param);
    Os_Arch_Dispatch();
    /* a pended switch happens here, once back at the caller's level */
    Os_Arch_ResumeInterrupts(&state);
    return res;
}

void Os_Arch_PrepareState(Os_TaskType task)
{
    /* started from scratch by PendSV, see Os_Arch_Switch */
    Os_Arch_Context[task] = NULL;
}

void Os_Arch_Init(void)
{
    Os_TaskType task;
    Os_IrqState state;

    Os_Arch_SuspendInterrupts(&state);

    for (task = 0u; task < OS_TASK_COUNT; ++task) {
        if (Os_TaskConfigs[task].stack_size < OS_ARCH_STACK_EXTRA + 16u) {
            exit(126); /* declare task stacks with OS_STACK_SIZE() */
        }
        Os_Arch_Context[task] = NULL;
    }
    Os_Arch_ContextNone = OS_ARCH_RUNNING;
    Os_Arch_Current     = &Os_Arch_ContextNone;

    OS_ARCH_SHPR_PENDSV  = 0xFFu;
    OS_ARCH_SHPR_SYSTICK = OS_ARCH_KERNEL_PRIORITY;

    OS_ARCH_SYST_CSR = 0u;
    OS_ARCH_SYST_RVR = OS_ARCH_TICK_CYCLES - 1u;
    OS_ARCH_SYST_CVR = 0u;
    OS_ARCH_SYST_CSR = OS_ARCH_SYST_CSR_START;
}

void Os_Arch_Start(void)
{
    const Os_IrqState enabled = 0u;

    /* the code calling Os_Start idles until shutdown. It keeps its stack,
     * now as the process stack, so PendSV saves and resumes it like any
     * task, and handlers move to a stack of their own */
    __asm__ __volatile__ ("mrs  r0, msp"     "\n\t"
                          "msr  psp, r0"     "\n\t"
                          "movs r0, #2"      "\n\t"
                          "msr  control, r0" "\n\t"
                          "isb"              "\n\t"
                          "msr  msp, %0"
                          :: "r"(&Os_Arch_HandlerStack[OS_ARCH_HANDLER_STACK_SIZE / sizeof(uint64_t)])
                          : "r0", "memory");

    /* switch to the first task if there is one, once interrupts are on */
    Os_Arch_Dispatch();
    Os_Arch_ResumeInterrupts(&enabled);
}
