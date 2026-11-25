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

/**
 * @file
 * @brief Vector table for the Cortex-M port with picolibc's crt0
 *
 * picolibc's crt0 provides the reset handler (_start), which sets up the
 * C runtime and calls main, and its linker script places this table, in
 * section .data.init.enter, at the start of flash where the core reads it
 * at reset. Faults end the program with exit code 128 plus the exception
 * number, as nothing sensible is left to return to.
 */

#include <stdint.h>
#include <stdlib.h>

#include "Os.h"

extern uint8_t __stack[];
extern void    _start(void);

static void Fault_Handler(void)
{
    uint32_t ipsr;
    __asm__ __volatile__ ("mrs %0, ipsr" : "=r"(ipsr));
    exit(128 + (int)(ipsr & 0xFFu));
}

typedef void (*Vector_Type)(void);

__attribute__((section(".data.init.enter"), used, aligned(128)))
const Vector_Type __interrupt_vector[16] = {
    (Vector_Type)(uintptr_t)__stack, /*  0 initial main stack pointer */
    _start,                          /*  1 reset */
    Fault_Handler,                   /*  2 NMI */
    Fault_Handler,                   /*  3 HardFault */
    Fault_Handler,                   /*  4 MemManage */
    Fault_Handler,                   /*  5 BusFault */
    Fault_Handler,                   /*  6 UsageFault */
    0, 0, 0, 0,                      /*  7-10 reserved */
    Fault_Handler,                   /* 11 SVCall, unused */
    Fault_Handler,                   /* 12 DebugMonitor */
    0,                               /* 13 reserved */
    Os_Arch_PendSVHandler,           /* 14 PendSV */
    Os_Arch_SysTickHandler,          /* 15 SysTick */
};
