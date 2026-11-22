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
 * FreeRTOS version of Os_MetricBasic: one task computing in a loop, so the
 * count shows what the tick costs a running task.
 */

#include "FreeRTOS_Metric.h"

volatile unsigned int task0_count;
unsigned int task0_array[1024];

static void task0(void* arg)
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

static void task_sup(void* arg)
{
    xTaskCreate(task0, "t0", 256, NULL, 1, NULL);
    FREERTOS_METRIC_RUN_ONE_SECOND();
    printf("Execution counts %u\n", task0_count);
    exit(0);
}

int main(void) { FreeRTOS_Metric_Start(); }
