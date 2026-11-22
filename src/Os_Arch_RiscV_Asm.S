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
 * Machine mode trap entry for the RISC-V port.
 *
 * A saved context is 32 words on the interrupted stack, indexed by register
 * number. The slots of x0, sp, gp and tp are not needed for registers, so
 * slot 0 holds mepc and slot 2 mstatus. gp and tp are the same for all
 * tasks and are left alone.
 *
 * The kernel runs on its own stack. Os_Arch_Trap gets the saved context and
 * returns the one to resume, which may belong to another task. A task that
 * starts from scratch has no saved context: Os_Arch_Trap then returns the
 * top of its stack with bit 0 set and its entry in mepc, and only the stack
 * pointer and the registers a C function expects at entry are set up.
 */

#define MSTATUS_START 0x1880 /* MPP machine mode, MPIE: interrupts enabled once started */

#if __riscv_xlen == 64
#define STORE   sd
#define LOAD    ld
#define REGBYTES 8
#else
#define STORE   sw
#define LOAD    lw
#define REGBYTES 4
#endif

#define FRAME   (32 * REGBYTES)
#define SLOT(n) ((n) * REGBYTES)

    .section .text.Os_Arch_TrapEntry, "ax", @progbits
    .globl  Os_Arch_TrapEntry
    .type   Os_Arch_TrapEntry, @function
    .align  4
Os_Arch_TrapEntry:
    addi    sp, sp, -FRAME
    STORE   x1,  SLOT(1)(sp)
    STORE   x5,  SLOT(5)(sp)
    STORE   x6,  SLOT(6)(sp)
    STORE   x7,  SLOT(7)(sp)
    STORE   x8,  SLOT(8)(sp)
    STORE   x9,  SLOT(9)(sp)
    STORE   x10, SLOT(10)(sp)
    STORE   x11, SLOT(11)(sp)
    STORE   x12, SLOT(12)(sp)
    STORE   x13, SLOT(13)(sp)
    STORE   x14, SLOT(14)(sp)
    STORE   x15, SLOT(15)(sp)
#ifndef __riscv_32e
    STORE   x16, SLOT(16)(sp)
    STORE   x17, SLOT(17)(sp)
    STORE   x18, SLOT(18)(sp)
    STORE   x19, SLOT(19)(sp)
    STORE   x20, SLOT(20)(sp)
    STORE   x21, SLOT(21)(sp)
    STORE   x22, SLOT(22)(sp)
    STORE   x23, SLOT(23)(sp)
    STORE   x24, SLOT(24)(sp)
    STORE   x25, SLOT(25)(sp)
    STORE   x26, SLOT(26)(sp)
    STORE   x27, SLOT(27)(sp)
    STORE   x28, SLOT(28)(sp)
    STORE   x29, SLOT(29)(sp)
    STORE   x30, SLOT(30)(sp)
    STORE   x31, SLOT(31)(sp)
#endif
    csrr    t0, mepc
    STORE   t0,  SLOT(0)(sp)
    csrr    t0, mstatus
    STORE   t0,  SLOT(2)(sp)

    /* run the kernel on its own stack */
    mv      a0, sp
    LOAD    sp, Os_Arch_KernelStackTop
    call    Os_Arch_Trap
    andi    t0, a0, 1
    bnez    t0, Os_Arch_TrapStart
    mv      sp, a0

    LOAD    t0,  SLOT(0)(sp)
    csrw    mepc, t0
    LOAD    t0,  SLOT(2)(sp)
    csrw    mstatus, t0
    LOAD    x1,  SLOT(1)(sp)
    LOAD    x5,  SLOT(5)(sp)
    LOAD    x6,  SLOT(6)(sp)
    LOAD    x7,  SLOT(7)(sp)
    LOAD    x8,  SLOT(8)(sp)
    LOAD    x9,  SLOT(9)(sp)
    LOAD    x10, SLOT(10)(sp)
    LOAD    x11, SLOT(11)(sp)
    LOAD    x12, SLOT(12)(sp)
    LOAD    x13, SLOT(13)(sp)
    LOAD    x14, SLOT(14)(sp)
    LOAD    x15, SLOT(15)(sp)
#ifndef __riscv_32e
    LOAD    x16, SLOT(16)(sp)
    LOAD    x17, SLOT(17)(sp)
    LOAD    x18, SLOT(18)(sp)
    LOAD    x19, SLOT(19)(sp)
    LOAD    x20, SLOT(20)(sp)
    LOAD    x21, SLOT(21)(sp)
    LOAD    x22, SLOT(22)(sp)
    LOAD    x23, SLOT(23)(sp)
    LOAD    x24, SLOT(24)(sp)
    LOAD    x25, SLOT(25)(sp)
    LOAD    x26, SLOT(26)(sp)
    LOAD    x27, SLOT(27)(sp)
    LOAD    x28, SLOT(28)(sp)
    LOAD    x29, SLOT(29)(sp)
    LOAD    x30, SLOT(30)(sp)
    LOAD    x31, SLOT(31)(sp)
#endif
    addi    sp, sp, FRAME
    mret

Os_Arch_TrapStart:
    andi    sp, a0, -2
    li      t0, MSTATUS_START
    csrw    mstatus, t0
    la      ra, Os_Arch_TaskReturned
    li      s0, 0               /* ends frame pointer backtraces at the task entry */
    mret
    .size   Os_Arch_TrapEntry, . - Os_Arch_TrapEntry
