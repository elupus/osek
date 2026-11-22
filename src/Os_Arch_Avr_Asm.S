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
 * System call and tick entry for the AVR port.
 *
 * AVR has no trap instruction, so Os_Arch_Syscall is an ordinary function.
 * Its caller already expects the call-clobbered registers to change, so it
 * only saves SREG and the call-saved registers r2-r17 and r28-r29, plus a
 * slot for the result, on the caller's stack:
 *
 *   high  return address (pushed by call)
 *         SREG, with the caller's interrupt flag
 *         r2 ... r17, r28, r29
 *   low   result, loaded into r24 on return       <- saved SP
 *
 * The Timer1 compare match A interrupt saves everything instead:
 *
 *   high  return address (pushed by the interrupt)
 *         r0, SREG, r1 ... r31
 *   low   RAMPZ                                   <- saved SP | 0x8000
 *
 * Bit 15 of the saved stack pointer tells the two apart, so stacks must lie
 * below 0x8000, which internal SRAM always does. EIND is set once by the
 * C runtime and never changed by compiled code, so it is not saved.
 *
 * Both switch to the kernel stack and call into C with interrupts disabled.
 * The C code returns the saved stack pointer of the context to resume, which
 * may belong to another task. Starting a task from scratch is done in C,
 * see Os_Arch_StartTask.
 */

#include <avr/io.h>

#ifdef __AVR_HAVE_JMP_CALL__
#define XCALL call
#define XJMP  jmp
#else
#define XCALL rcall
#define XJMP  rjmp
#endif

#define IO(reg) _SFR_IO_ADDR(reg)

    .section .text.Os_Arch_Syscall, "ax", @progbits
    .global Os_Arch_Syscall
    .type   Os_Arch_Syscall, @function
Os_Arch_Syscall:
    in      r0, IO(SREG)
    cli
    push    r0
    push    r2
    push    r3
    push    r4
    push    r5
    push    r6
    push    r7
    push    r8
    push    r9
    push    r10
    push    r11
    push    r12
    push    r13
    push    r14
    push    r15
    push    r16
    push    r17
    push    r28
    push    r29
    push    r24                 /* result slot */

    /* Os_Arch_SyscallHandler(param in r24:r25, frame in r22:r23) */
    in      r22, IO(SPL)
    in      r23, IO(SPH)
    lds     r30, Os_Arch_KernelStackTop
    lds     r31, Os_Arch_KernelStackTop + 1
    out     IO(SPH), r31
    out     IO(SPL), r30
    XCALL   Os_Arch_SyscallHandler

    /* resume the context whose saved stack pointer is in r24:r25 */
Os_Arch_Restore:
    sbrc    r25, 7
    rjmp    Os_Arch_RestoreFull
    out     IO(SPH), r25
    out     IO(SPL), r24
    pop     r24                 /* result */
    pop     r29
    pop     r28
    pop     r17
    pop     r16
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     r11
    pop     r10
    pop     r9
    pop     r8
    pop     r7
    pop     r6
    pop     r5
    pop     r4
    pop     r3
    pop     r2
    pop     r0
    out     IO(SREG), r0        /* r1 is zero from the kernel, as C expects */
    ret

Os_Arch_RestoreFull:
    andi    r25, 0x7f
    out     IO(SPH), r25
    out     IO(SPL), r24
#ifdef __AVR_HAVE_RAMPZ__
    pop     r0
    out     IO(RAMPZ), r0
#endif
    pop     r31
    pop     r30
    pop     r29
    pop     r28
    pop     r27
    pop     r26
    pop     r25
    pop     r24
    pop     r23
    pop     r22
    pop     r21
    pop     r20
    pop     r19
    pop     r18
    pop     r17
    pop     r16
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     r11
    pop     r10
    pop     r9
    pop     r8
    pop     r7
    pop     r6
    pop     r5
    pop     r4
    pop     r3
    pop     r2
    pop     r1
    pop     r0
    out     IO(SREG), r0
    pop     r0
    reti
    .size   Os_Arch_Syscall, . - Os_Arch_Syscall

    .section .text.Os_Arch_TickEntry, "ax", @progbits
    .global TIMER1_COMPA_vect
    .type   TIMER1_COMPA_vect, @function
TIMER1_COMPA_vect:
    push    r0
    in      r0, IO(SREG)
    push    r0
    push    r1
    clr     r1
    push    r2
    push    r3
    push    r4
    push    r5
    push    r6
    push    r7
    push    r8
    push    r9
    push    r10
    push    r11
    push    r12
    push    r13
    push    r14
    push    r15
    push    r16
    push    r17
    push    r18
    push    r19
    push    r20
    push    r21
    push    r22
    push    r23
    push    r24
    push    r25
    push    r26
    push    r27
    push    r28
    push    r29
    push    r30
    push    r31
#ifdef __AVR_HAVE_RAMPZ__
    in      r0, IO(RAMPZ)
    push    r0
#endif

    /* Os_Arch_TickHandler(frame in r24:r25), tagged as a full context */
    in      r24, IO(SPL)
    in      r25, IO(SPH)
    ori     r25, 0x80
    lds     r30, Os_Arch_KernelStackTop
    lds     r31, Os_Arch_KernelStackTop + 1
    out     IO(SPH), r31
    out     IO(SPL), r30
    XCALL   Os_Arch_TickHandler
    XJMP    Os_Arch_Restore
    .size   TIMER1_COMPA_vect, . - TIMER1_COMPA_vect
