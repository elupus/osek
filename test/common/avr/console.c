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
 * Console and exit for applications on an ATmega with USART0, such as
 * QEMU's mega2560 machine, which has no semihosting.
 *
 * stdout and stderr go to USART0. exit(), which returning from main also
 * calls, is wrapped (link with -Wl,--wrap=exit) to print a final line
 * "OSEK_EXIT <code>" and stop. toolchain/avr-qemu-run.py waits for that
 * line and exits QEMU with the code.
 */

#include <stdio.h>
#include <avr/io.h>

#ifndef CONSOLE_BAUD
#define CONSOLE_BAUD 115200UL
#endif

#ifndef CONSOLE_CPU_HZ
#ifdef F_CPU
#define CONSOLE_CPU_HZ F_CPU
#else
#define CONSOLE_CPU_HZ 16000000UL
#endif
#endif

static int console_putc(char c, FILE* stream)
{
    (void)stream;
    if (c == '\n') {
        console_putc('\r', stream);
    }
    loop_until_bit_is_set(UCSR0A, UDRE0);
    UDR0 = (uint8_t)c;
    return 0;
}

static FILE console = FDEV_SETUP_STREAM(console_putc, NULL, _FDEV_SETUP_WRITE);

/* before main, after the C runtime has set up the data sections */
static void __attribute__((constructor, used)) console_init(void)
{
    UBRR0  = (uint16_t)(CONSOLE_CPU_HZ / 8u / CONSOLE_BAUD - 1u);
    UCSR0A = _BV(U2X0);
    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00); /* 8N1 */
    UCSR0B = _BV(TXEN0);
    stdout = &console;
    stderr = &console;
}

void __wrap_exit(int code) __attribute__((noreturn));

void __wrap_exit(int code)
{
    __asm__ __volatile__ ("cli" ::: "memory");
    printf("\nOSEK_EXIT %d\n", code);
    for (;;) {
        /* interrupts are disabled, so this sleeps for good */
        __asm__ __volatile__ ("sleep" ::: "memory");
    }
}
