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
 * FreeRTOS version of Os_MetricPreemptive.
 */

#include "FreeRTOS_Metric.h"

/* task0 runs at the lowest priority and wakes task 1, which preempts it and
 * wakes task 2, up to task 4 (all task_n): the OSEK app's ActivateTask
 * chain, with an activation mapped to a task notification and TerminateTask
 * to waiting */
volatile unsigned int task_count[5];
static TaskHandle_t   task_handle[5];

static void task0(void* arg)
{
    while (1) {
        task_count[0]++;
        xTaskNotifyGive(task_handle[1]);
    }
}

static void task_n(void* arg)
{
    int n = (int)(intptr_t)arg;
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        task_count[n]++;
        if (n < 4) {
            xTaskNotifyGive(task_handle[n + 1]);
        }
    }
}

static void task_sup(void* arg)
{
    unsigned int total = 0;
    for (int n = 4; n >= 1; --n) {
        xTaskCreate(task_n, "t", 256, (void*)(intptr_t)n, 1 + n, &task_handle[n]);
    }
    xTaskCreate(task0, "t0", 256, NULL, 1, &task_handle[0]);
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
