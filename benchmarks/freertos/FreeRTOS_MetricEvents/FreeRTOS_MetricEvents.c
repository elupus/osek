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
 * FreeRTOS version of Os_MetricEvents: a task notification ping-pong.
 * Each round is a wake up and a block, two task switches without any task
 * restart, like the OSEK event ping-pong.
 */

#include "FreeRTOS_Metric.h"

/* task_lo (prio 1) wakes task_hi (prio 2) with a notification; task_hi
 * takes it and waits again */
volatile unsigned int task_lo_count, task_hi_count;
static TaskHandle_t task_hi_handle;

static void task_lo(void* arg)
{
    while (1) {
        task_lo_count++;
        xTaskNotifyGive(task_hi_handle);
    }
}

static void task_hi(void* arg)
{
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        task_hi_count++;
    }
}

static void task_sup(void* arg)
{
    xTaskCreate(task_hi, "hi", 256, NULL, 2, &task_hi_handle);
    xTaskCreate(task_lo, "lo", 256, NULL, 1, NULL);
    FREERTOS_METRIC_RUN_ONE_SECOND();
    printf("Execution counts %u (%u, %u)\n", task_hi_count, task_lo_count, task_hi_count);
    exit(0);
}

int main(void) { FreeRTOS_Metric_Start(); }
