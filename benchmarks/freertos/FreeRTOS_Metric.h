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
 * Start up shared by the FreeRTOS metric apps: installs the trap handler,
 * creates the app's supervisor task task_sup at the highest priority and
 * starts the scheduler. The supervisor starts the test, lets it run for one
 * second plus a tick, then prints the counts and exits through semihosting.
 */

#ifndef FREERTOS_METRIC_H_
#define FREERTOS_METRIC_H_

#include <stdio.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"

extern void freertos_risc_v_trap_handler(void);

/* stops the run after one second plus a tick, like the OSEK alarm */
static void task_sup(void* arg);

static void FreeRTOS_Metric_Start(void)
{
    __asm__ volatile ("csrw mtvec, %0" :: "r"(freertos_risc_v_trap_handler));
    xTaskCreate(task_sup, "sup", 1024, NULL, configMAX_PRIORITIES - 1, NULL);
    vTaskStartScheduler();
    exit(1);
}

#define FREERTOS_METRIC_RUN_ONE_SECOND() vTaskDelay(configTICK_RATE_HZ + 1)

#endif /* FREERTOS_METRIC_H_ */
