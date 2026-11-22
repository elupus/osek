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
 * FreeRTOS version of Os_MetricCooperative: five tasks of equal priority
 * that count and yield in turn. taskYIELD only switches to the next task,
 * while Os_ChainTask terminates and restarts the task, so the two are not
 * the same operation.
 */

#include "FreeRTOS_Metric.h"

volatile unsigned int task_count[5];

static void task_n(void* arg)
{
    volatile unsigned int* count = arg;
    while (1) {
        (*count)++;
        taskYIELD();
    }
}

static void task_sup(void* arg)
{
    unsigned int total = 0;
    for (int i = 0; i < 5; ++i) {
        xTaskCreate(task_n, "t", 256, (void*)&task_count[i], 1, NULL);
    }
    FREERTOS_METRIC_RUN_ONE_SECOND();
    for (int i = 0; i < 5; ++i) {
        total += task_count[i];
    }
    printf("Execution counts %u (%u, %u, %u, %u, %u)\n", total / 5,
           task_count[0], task_count[1], task_count[2], task_count[3],
           task_count[4]);
    exit(0);
}

int main(void) { FreeRTOS_Metric_Start(); }
