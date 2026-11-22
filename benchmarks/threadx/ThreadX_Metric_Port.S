/* OSEKOS Implementation of an OSEK Scheduler
 * Copyright (C) 2026 Joakim Plate
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
 *
 * What ThreadX leaves to the application on QEMU's virt machine, after the
 * port's qemu_virt example: _tx_initialize_low_level, the trap entry, and
 * the 10 ms tick from the CLINT machine timer (10 MHz), the same tick as
 * the OSEK and FreeRTOS metric apps. Only the timer interrupt is expected;
 * any other trap ends the run with exit code 1.
 */

#include "tx_port.h"

#define CLINT_MTIMECMP  0x02004000      /* hart 0 */
#define CLINT_MTIME     0x0200BFF8
#define TICK_PERIOD     100000          /* 10 MHz / 100 Hz */
#define MCAUSE_MTI      0x80000007      /* machine timer interrupt */
#define MIE_MTIE        0x80

    .section .text

/* Called by tx_kernel_enter with interrupts disabled. Saves the system
 * stack, which the interrupts run on, installs the trap entry and starts
 * the tick. The apps allocate no memory from ThreadX, so
 * _tx_initialize_unused_memory stays NULL. */
    .global _tx_initialize_low_level
_tx_initialize_low_level:
    la      t0, _tx_thread_system_stack_ptr
    sw      sp, 0(t0)

    la      t0, ThreadX_Metric_TrapEntry
    csrw    mtvec, t0

    /* mtimecmp = mtime + TICK_PERIOD, high word parked at all ones so that
     * the compare never passes below the timer half way through */
    li      t0, CLINT_MTIME
1:  lw      t2, 4(t0)
    lw      t1, 0(t0)
    lw      t3, 4(t0)
    bne     t2, t3, 1b
    li      t0, CLINT_MTIMECMP
    li      t3, TICK_PERIOD
    add     t4, t1, t3
    sltu    t5, t4, t1
    add     t2, t2, t5
    li      t6, -1
    sw      t6, 4(t0)
    sw      t4, 0(t0)
    sw      t2, 4(t0)

    li      t0, MIE_MTIE
    csrs    mie, t0
    ret

/* The trap entry of the port's examples: save the interrupted context,
 * handle the tick, and let ThreadX return to the thread that is to run. */
    .align  2
ThreadX_Metric_TrapEntry:
    addi    sp, sp, -TX_RISCV_TRAP_FRAME_SIZE
    sw      ra, 28*4(sp)
    call    _tx_thread_context_save

    csrr    t0, mcause
    li      t1, MCAUSE_MTI
    bne     t0, t1, ThreadX_Metric_Fault

    /* next tick one period after the last one: mtimecmp += TICK_PERIOD */
    li      t0, CLINT_MTIMECMP
    lw      t1, 0(t0)
    lw      t2, 4(t0)
    li      t3, TICK_PERIOD
    add     t4, t1, t3
    sltu    t5, t4, t1
    add     t2, t2, t5
    li      t6, -1
    sw      t6, 4(t0)
    sw      t4, 0(t0)
    sw      t2, 4(t0)

    call    _tx_timer_interrupt
    j       _tx_thread_context_restore

ThreadX_Metric_Fault:
    li      a0, 1
    call    exit
