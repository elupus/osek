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
 * Start up shared by the ThreadX metric apps: ThreadX_Metric_Start enters
 * the kernel, which calls tx_application_define, which creates the app's
 * supervisor thread task_sup at the highest priority, 0. The supervisor
 * starts the test, lets it run for one second plus a tick, then prints the
 * counts and exits through semihosting. ThreadX_Metric_Port.S has the low
 * level start up, trap entry and tick.
 */

#ifndef THREADX_METRIC_H_
#define THREADX_METRIC_H_

#include <stdio.h>
#include <stdlib.h>
#include "tx_api.h"

/** a thread and its stack, allocated statically by the apps */
typedef struct {
    TX_THREAD thread;
    ULONG     stack[1024 / sizeof(ULONG)];
} ThreadX_Metric_ThreadType;

/* stops the run after one second plus a tick, like the OSEK alarm */
static void task_sup(ULONG arg);

/**
 * @brief Create and start a thread without time slicing
 * @param t        thread and stack
 * @param entry    thread function
 * @param arg      argument of the thread function
 * @param priority 0 is the highest
 */
static void ThreadX_Metric_CreateThread(ThreadX_Metric_ThreadType* t,
                                        void (*entry)(ULONG), ULONG arg,
                                        UINT priority)
{
    tx_thread_create(&t->thread, "t", entry, arg, t->stack, sizeof(t->stack),
                     priority, priority, TX_NO_TIME_SLICE, TX_AUTO_START);
}

void tx_application_define(void* first_unused_memory)
{
    static ThreadX_Metric_ThreadType sup;
    (void)first_unused_memory;
    ThreadX_Metric_CreateThread(&sup, task_sup, 0, 0);
}

#define THREADX_METRIC_RUN_ONE_SECOND() tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND + 1)

static void ThreadX_Metric_Start(void)
{
    tx_kernel_enter();
    exit(1);
}

#endif /* THREADX_METRIC_H_ */
