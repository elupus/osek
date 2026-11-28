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
#include <avr/io.h>

#include "Os.h"

/* marks the saved stack pointer of a context saved by the tick, see
 * Os_Arch_Avr_Asm.S */
#define OS_ARCH_FRAME_FULL 0x8000u

/* Timer1 in CTC mode, with the smallest prescaler that fits the period */
#define OS_ARCH_TICK_CYCLES ((OS_ARCH_CPU_HZ / 1000UL) * OS_TICK_US / 1000UL)

#if   OS_ARCH_TICK_CYCLES / 1u    <= 0x10000UL
#define OS_ARCH_TICK_PRESCALE 1u
#define OS_ARCH_TICK_CS       (_BV(CS10))
#elif OS_ARCH_TICK_CYCLES / 8u    <= 0x10000UL
#define OS_ARCH_TICK_PRESCALE 8u
#define OS_ARCH_TICK_CS       (_BV(CS11))
#elif OS_ARCH_TICK_CYCLES / 64u   <= 0x10000UL
#define OS_ARCH_TICK_PRESCALE 64u
#define OS_ARCH_TICK_CS       (_BV(CS11) | _BV(CS10))
#elif OS_ARCH_TICK_CYCLES / 256u  <= 0x10000UL
#define OS_ARCH_TICK_PRESCALE 256u
#define OS_ARCH_TICK_CS       (_BV(CS12))
#elif OS_ARCH_TICK_CYCLES / 1024u <= 0x10000UL
#define OS_ARCH_TICK_PRESCALE 1024u
#define OS_ARCH_TICK_CS       (_BV(CS12) | _BV(CS10))
#else
#error "OS_TICK_US is too long for Timer1"
#endif

#if OS_ARCH_TICK_CYCLES / OS_ARCH_TICK_PRESCALE == 0u
#error "OS_TICK_US is shorter than one CPU cycle"
#endif

extern volatile boolean Os_Continue;

static uint8_t Os_Arch_KernelStack[OS_ARCH_KERNEL_STACK_SIZE];

/** top of the kernel stack, loaded by the system call and tick entries. The
 * stack pointer addresses the next free byte */
uint8_t* const Os_Arch_KernelStackTop = &Os_Arch_KernelStack[OS_ARCH_KERNEL_STACK_SIZE - 1u];

/** saved stack pointer of each task, 0 for a task to start from scratch,
 * and of the code that called Os_Start */
static uint16_t Os_Arch_Context[OS_TASK_COUNT];
static uint16_t Os_Arch_ContextNone;

/** context slot of the code running now. The kernel's view can differ, as
 * Os_Start schedules the first task before switching to it */
static uint16_t* Os_Arch_Current;

uint16_t Os_Arch_SyscallHandler(Os_SyscallParamType* param, uint16_t frame);
uint16_t Os_Arch_TickHandler(uint16_t frame);

/* a task function returned instead of terminating */
void Os_Arch_TaskReturned(void)
{
    exit(127);
}

/**
 * @brief Start a task from scratch, on its empty stack
 *
 * Only the stack pointer and the entry matter to a C function, so no
 * context is prepared: the stack gets Os_Arch_TaskReturned as return
 * address and the entry below it, which reti jumps to with interrupts
 * enabled. Function pointers are word addresses, with EIND as the high
 * byte on parts with a 3 byte program counter.
 */
static void __attribute__((noreturn)) Os_Arch_StartTask(Os_TaskType task)
{
    const Os_TaskConfigType* cfg = &Os_TaskConfigs[task];
    uint8_t* top   = (uint8_t*)cfg->stack + cfg->stack_size - 1u;
    uint16_t entry = (uint16_t)cfg->entry;
    uint16_t ret   = (uint16_t)Os_Arch_TaskReturned;

    __asm__ __volatile__ (
        "out  __SP_H__, %B0"   "\n\t"
        "out  __SP_L__, %A0"   "\n\t"
        "push %A2"             "\n\t"
        "push %B2"             "\n\t"
#if defined(__AVR_3_BYTE_PC__)
        "in   __tmp_reg__, %3" "\n\t"
        "push __tmp_reg__"     "\n\t"
#endif
        "push %A1"             "\n\t"
        "push %B1"             "\n\t"
#if defined(__AVR_3_BYTE_PC__)
        "push __tmp_reg__"     "\n\t"
#endif
        "reti"
        :
        : "r"(top), "r"(entry), "r"(ret)
#if defined(__AVR_3_BYTE_PC__)
        , "I"(_SFR_IO_ADDR(EIND))
#endif
        : "memory");
    __builtin_unreachable();
}

/**
 * @brief Pick the context to resume once the kernel is done
 * @return its saved stack pointer, does not return for a task to start
 *         from scratch
 */
static uint16_t Os_Arch_Resume(void)
{
    Os_TaskType task;

    (void)Os_GetTaskId(&task);
    if (task == OS_INVALID_TASK) {
        Os_Arch_Current = &Os_Arch_ContextNone;
    } else {
        Os_Arch_Current = &Os_Arch_Context[task];
        if (*Os_Arch_Current == 0u) {
            Os_Arch_StartTask(task);
        }
    }
    return *Os_Arch_Current;
}

/**
 * @brief Handle a system call, called by Os_Arch_Syscall on the kernel stack
 * @param param system call parameters, NULL to only reschedule
 * @param frame saved stack pointer of the caller
 * @return saved stack pointer of the context to resume
 */
uint16_t Os_Arch_SyscallHandler(Os_SyscallParamType* param, uint16_t frame)
{
    Os_StatusType res = E_OK;

    *Os_Arch_Current = frame;
    if (param) {
        res = Os_Syscall_Internal(param);
    }

    /* return the status in the result slot, unless the caller was
     * restarted (ChainTask to itself) and has no saved context left */
    if (*Os_Arch_Current == frame) {
        ((uint8_t*)frame)[1] = res;
    }
    return Os_Arch_Resume();
}

/**
 * @brief Handle the tick, called by the Timer1 compare match A interrupt
 *        on the kernel stack
 * @param frame saved stack pointer of the interrupted code
 * @return saved stack pointer of the context to resume
 */
uint16_t Os_Arch_TickHandler(uint16_t frame)
{
    *Os_Arch_Current = frame;
    if (Os_Continue) {
        Os_Isr();
    } else {
        /* shut down, stop the tick so no task runs again */
        TIMSK1 = 0u;
    }
    return Os_Arch_Resume();
}

void Os_Arch_PrepareState(Os_TaskType task)
{
    /* started from scratch, see Os_Arch_StartTask */
    Os_Arch_Context[task] = 0u;
}

void Os_Arch_Init(void)
{
    Os_TaskType task;

    Os_Arch_DisableAllInterrupts();

    for (task = 0u; task < OS_TASK_COUNT; ++task) {
        if (Os_TaskConfigs[task].stack_size < OS_ARCH_STACK_EXTRA + 16u) {
            exit(126); /* declare task stacks with OS_STACK_SIZE() */
        }
        if ((uint16_t)Os_TaskConfigs[task].stack + Os_TaskConfigs[task].stack_size > OS_ARCH_FRAME_FULL) {
            exit(125); /* bit 15 of a saved stack pointer marks its kind */
        }
        Os_Arch_Context[task] = 0u;
    }
    Os_Arch_ContextNone = 0u;
    Os_Arch_Current     = &Os_Arch_ContextNone;

    /* idle sleep mode, so Os_Arch_Wait keeps the timer running */
    SMCR = _BV(SE);

    TCCR1A = 0u;
    TCCR1B = 0u;
    TCNT1  = 0u;
    OCR1A  = (uint16_t)(OS_ARCH_TICK_CYCLES / OS_ARCH_TICK_PRESCALE - 1u);
    TIFR1  = _BV(OCF1A);
    TIMSK1 = _BV(OCIE1A);
    TCCR1B = _BV(WGM12) | OS_ARCH_TICK_CS;
}

void Os_Arch_Start(void)
{
    /* enable interrupts for the code calling Os_Start, which idles until
     * shutdown, and switch to the first task if there is one */
    Os_Arch_EnableAllInterrupts();
    (void)Os_Arch_Syscall(NULL);
}
