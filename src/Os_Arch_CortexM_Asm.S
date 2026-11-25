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

/*
 * PendSV handler of the Cortex-M port, which does every context switch.
 *
 * Every context runs in thread mode on the process stack. On exception
 * entry the core saves r0-r3, r12, lr, pc and xPSR there, and this handler
 * saves r4-r11 below them:
 *
 *   high  xPSR, pc, lr, r12, r3, r2, r1, r0   (exception frame)
 *   low   r11 ... r4                          <- saved stack pointer
 *
 * Os_Arch_Switch records the saved stack pointer, unless the task restarted
 * itself, and returns the one to resume. A task that starts from scratch
 * has no r4-r11 to restore: Os_Arch_Switch then returns its exception frame
 * with bit 0 set, which only holds the entry, xPSR and the return address.
 *
 * Every context returns to thread mode on the process stack without
 * floating point state, EXC_RETURN 0xFFFFFFFD.
 */

    .syntax unified
    .thumb

    .section .text.Os_Arch_PendSVHandler, "ax", %progbits
    .globl  Os_Arch_PendSVHandler
    .type   Os_Arch_PendSVHandler, %function
    .thumb_func
Os_Arch_PendSVHandler:
    mrs     r0, psp
    stmdb   r0!, {r4-r11}
    bl      Os_Arch_Switch
    mvn     lr, #2                  /* EXC_RETURN 0xFFFFFFFD */
    tst     r0, #1
    bne     1f
    ldmia   r0!, {r4-r11}
    msr     psp, r0
    bx      lr
1:  bic     r0, r0, #1              /* fresh start, r4-r11 are left as they are */
    msr     psp, r0
    bx      lr
    .size   Os_Arch_PendSVHandler, . - Os_Arch_PendSVHandler
