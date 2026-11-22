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
 * ThreadX version of Os_MetricCooperative: five threads of equal priority,
 * without time slicing, that count and relinquish in turn.
 * tx_thread_relinquish only switches to the next thread, while
 * Os_ChainTask terminates and restarts the task, so the two are not the
 * same operation.
 */

#include "ThreadX_Metric.h"

volatile unsigned int task_count[5];
static ThreadX_Metric_ThreadType task_thread[5];

static void task_n(ULONG n)
{
    while (1) {
        task_count[n]++;
        tx_thread_relinquish();
    }
}

static void task_sup(ULONG arg)
{
    unsigned int total = 0;
    for (int i = 0; i < 5; ++i) {
        ThreadX_Metric_CreateThread(&task_thread[i], task_n, i, 1);
    }
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
