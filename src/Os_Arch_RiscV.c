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

/* saved context slots, see Os_Arch_RiscV_Asm.S */
#define OS_ARCH_FRAME_MEPC     0u
#define OS_ARCH_FRAME_RA       1u
#define OS_ARCH_FRAME_MSTATUS  2u
#define OS_ARCH_FRAME_FP       8u   /* s0 */
#define OS_ARCH_FRAME_A0       10u

#define OS_ARCH_MSTATUS_MPIE   0x80u
#define OS_ARCH_MSTATUS_MPP_M  0x1800u
#define OS_ARCH_MIE_MTIE       0x80u

#define OS_ARCH_MCAUSE_INTERRUPT ((uintptr_t)1u << (sizeof(uintptr_t) * 8u - 1u))
#define OS_ARCH_MCAUSE_MTI       7u   /**< machine timer interrupt */
#define OS_ARCH_MCAUSE_ECALL_M   11u  /**< ecall from machine mode */

#define OS_ARCH_MTIMECMP (OS_ARCH_CLINT_BASE + 0x4000u) /* hart 0 */
#define OS_ARCH_MTIME    (OS_ARCH_CLINT_BASE + 0xBFF8u)

#define OS_ARCH_TICK_PERIOD ((uint64_t)OS_ARCH_MTIME_HZ * OS_TICK_US / 1000000u)

#if ((OS_ARCH_MTIME_HZ / 1000u) * OS_TICK_US / 1000u == 0u)
#error "OS_TICK_US is shorter than one machine timer tick"
#endif

void Os_Arch_TrapEntry(void);

extern volatile boolean Os_Continue;

static uintptr_t Os_Arch_KernelStack[OS_ARCH_KERNEL_STACK_SIZE / sizeof(uintptr_t)] __attribute__((aligned(16)));

/** top of the kernel stack, loaded by the trap entry */
uintptr_t* const Os_Arch_KernelStackTop = &Os_Arch_KernelStack[OS_ARCH_KERNEL_STACK_SIZE / sizeof(uintptr_t)];

/** saved context of each task, and of the code that called Os_Start */
static uintptr_t* Os_Arch_Context[OS_TASK_COUNT];
static uintptr_t* Os_Arch_ContextNone;

/** context slot of the code running now. The kernel's view can differ, as
 * Os_Start schedules the first task before switching to it */
static uintptr_t** Os_Arch_Current;

/** machine timer value of the next tick */
static uint64_t Os_Arch_NextTick;

static uintptr_t** Os_Arch_GetContext(void)
{
    Os_TaskType task;
    (void)Os_GetTaskId(&task);
    if (task == OS_INVALID_TASK) {
        return &Os_Arch_ContextNone;
    }
    return &Os_Arch_Context[task];
}

static uint64_t Os_Arch_ReadMtime(void)
{
#if __riscv_xlen == 64
    return *(volatile uint64_t*)OS_ARCH_MTIME;
#else
    volatile uint32* mtime = (volatile uint32*)OS_ARCH_MTIME;
    uint32 hi, lo;
    do {
        hi = mtime[1];
        lo = mtime[0];
    } while (hi != mtime[1]);
    return ((uint64_t)hi << 32) | lo;
#endif
}

static void Os_Arch_WriteMtimecmp(uint64_t value)
{
#if __riscv_xlen == 64
    *(volatile uint64_t*)OS_ARCH_MTIMECMP = value;
#else
    /* never let the compare value pass below the timer half way through */
    volatile uint32* cmp = (volatile uint32*)OS_ARCH_MTIMECMP;
    cmp[1] = 0xFFFFFFFFu;
    cmp[0] = (uint32)value;
    cmp[1] = (uint32)(value >> 32);
#endif
}

/**
 * @brief Handle a trap, called by the trap entry on the kernel stack
 * @param frame saved context of the interrupted code
 * @return saved context to resume
 */
uintptr_t* Os_Arch_Trap(uintptr_t* frame)
{
    uintptr_t cause;

    __asm__ __volatile__ ("csrr %0, mcause" : "=r"(cause));

    *Os_Arch_Current = frame;

    if (cause == (OS_ARCH_MCAUSE_INTERRUPT | OS_ARCH_MCAUSE_MTI)) {
        if (Os_Continue) {
            Os_Arch_NextTick += OS_ARCH_TICK_PERIOD;
            Os_Arch_WriteMtimecmp(Os_Arch_NextTick);
            Os_Isr();
        } else {
            /* shut down, stop the tick so no task runs again */
            __asm__ __volatile__ ("csrc mie, %0" :: "r"(OS_ARCH_MIE_MTIE));
        }
    } else if (cause == OS_ARCH_MCAUSE_ECALL_M) {
        Os_SyscallParamType* param = (Os_SyscallParamType*)frame[OS_ARCH_FRAME_A0];
        Os_StatusType        res   = E_OK; /* NULL only reschedules, see Os_Arch_Start */

        frame[OS_ARCH_FRAME_MEPC] += 4u; /* continue after the ecall */
        if (param) {
            res = Os_Syscall_Internal(param);
        }

        /* return the status, unless the caller was restarted (ChainTask to
         * itself), whose new context may overlap the one saved here */
        if (*Os_Arch_Current == frame) {
            frame[OS_ARCH_FRAME_A0] = res;
        }
    } else {
        /* an exception, nothing sensible to return to */
        exit(128 + (int)(cause & 0xFFu));
    }

    Os_Arch_Current = Os_Arch_GetContext();
    return *Os_Arch_Current;
}

Os_StatusType Os_Arch_Syscall(Os_SyscallParamType* param)
{
    register uintptr_t a0 __asm__("a0") = (uintptr_t)param;
    __asm__ __volatile__ ("ecall" : "+r"(a0) :: "memory");
    return (Os_StatusType)a0;
}

/* a task function returned instead of terminating */
static void Os_Arch_TaskReturned(void)
{
    exit(127);
}

void Os_Arch_PrepareState(Os_TaskType task)
{
    uintptr_t  top   = ((uintptr_t)Os_TaskConfigs[task].stack + Os_TaskConfigs[task].stack_size) & ~(uintptr_t)15u;
    uintptr_t* frame = (uintptr_t*)top - OS_ARCH_FRAME_WORDS;

    /* only these matter to a task starting in a C function without
     * arguments, the others are scratch or restored only on its return */
    frame[OS_ARCH_FRAME_MEPC]    = (uintptr_t)Os_TaskConfigs[task].entry;
    frame[OS_ARCH_FRAME_RA]      = (uintptr_t)Os_Arch_TaskReturned;
    frame[OS_ARCH_FRAME_MSTATUS] = OS_ARCH_MSTATUS_MPP_M | OS_ARCH_MSTATUS_MPIE; /* starts with interrupts enabled */
    frame[OS_ARCH_FRAME_FP]      = 0u; /* ends frame pointer backtraces at the task entry */
    Os_Arch_Context[task] = frame;
}

void Os_Arch_Init(void)
{
    Os_TaskType task;

    Os_Arch_DisableAllInterrupts();

    for (task = 0u; task < OS_TASK_COUNT; ++task) {
        if (Os_TaskConfigs[task].stack_size < OS_ARCH_STACK_EXTRA + 16u) {
            exit(126); /* declare task stacks with OS_STACK_SIZE() */
        }
        Os_Arch_Context[task] = NULL;
    }
    Os_Arch_ContextNone = NULL;
    Os_Arch_Current     = &Os_Arch_ContextNone;

    __asm__ __volatile__ ("csrw mtvec, %0" :: "r"((uintptr_t)Os_Arch_TrapEntry));

    Os_Arch_NextTick = Os_Arch_ReadMtime() + OS_ARCH_TICK_PERIOD;
    Os_Arch_WriteMtimecmp(Os_Arch_NextTick);
    __asm__ __volatile__ ("csrs mie, %0" :: "r"(OS_ARCH_MIE_MTIE));
}

void Os_Arch_Start(void)
{
    /* enable interrupts for the code calling Os_Start, which idles until
     * shutdown, and switch to the first task if there is one */
    Os_Arch_EnableAllInterrupts();
    (void)Os_Arch_Syscall(NULL);
}
