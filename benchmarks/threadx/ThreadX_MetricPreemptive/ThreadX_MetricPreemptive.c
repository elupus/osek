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
 * ThreadX version of Os_MetricPreemptive.
 */

#include "ThreadX_Metric.h"

/* task0 runs at the lowest priority and wakes task 1, which preempts it and
 * wakes task 2, up to task 4 (all task_n): the OSEK app's ActivateTask
 * chain, with an activation mapped to a semaphore put and TerminateTask to
 * waiting on the semaphore. ThreadX priorities are reversed, 0 is the
 * highest: task n runs at 5 - n, below the supervisor at 0. */
volatile unsigned int task_count[5];
static ThreadX_Metric_ThreadType task_thread[5];
static TX_SEMAPHORE task_sem[5];

static void task0(ULONG arg)
{
    while (1) {
        task_count[0]++;
        tx_semaphore_put(&task_sem[1]);
    }
}

static void task_n(ULONG n)
{
    while (1) {
        tx_semaphore_get(&task_sem[n], TX_WAIT_FOREVER);
        task_count[n]++;
        if (n < 4) {
            tx_semaphore_put(&task_sem[n + 1]);
        }
    }
}

static void task_sup(ULONG arg)
{
    unsigned int total = 0;
    for (int n = 4; n >= 1; --n) {
        tx_semaphore_create(&task_sem[n], "s", 0);
        ThreadX_Metric_CreateThread(&task_thread[n], task_n, n, 5 - n);
    }
    ThreadX_Metric_CreateThread(&task_thread[0], task0, 0, 5);
    THREADX_METRIC_RUN_ONE_SECOND();
    for (int i = 0; i < 5; ++i) {
        total += task_count[i];
    }
    printf("Execution counts %u (%u, %u, %u, %u, %u)\n", total / 5,
           task_count[0], task_count[1], task_count[2], task_count[3],
           task_count[4]);
    exit(0);
}

int main(void) { ThreadX_Metric_Start(); }
