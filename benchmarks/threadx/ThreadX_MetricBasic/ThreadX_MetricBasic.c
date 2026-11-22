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
 * ThreadX version of Os_MetricBasic: one thread computing in a loop, so
 * the count shows what the tick costs a running thread.
 */

#include "ThreadX_Metric.h"

volatile unsigned int task0_count;
unsigned int task0_array[1024];
static ThreadX_Metric_ThreadType task0_thread;

static void task0(ULONG arg)
{
    int i;
    for (i = 0; i < 1024; ++i) {
        task0_array[i] = 0u;
    }
    while (1) {
        for (i = 0; i < 1024; ++i) {
            task0_array[i] = (task0_array[i] + task0_count) ^ task0_array[i];
        }
        task0_count++;
    }
}

static void task_sup(ULONG arg)
{
    ThreadX_Metric_CreateThread(&task0_thread, task0, 0, 1);
    THREADX_METRIC_RUN_ONE_SECOND();
    printf("Execution counts %u\n", task0_count);
    exit(0);
}

int main(void) { ThreadX_Metric_Start(); }
