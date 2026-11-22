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
 * FreeRTOS configuration for the metric apps on QEMU's virt machine, with
 * the same tick and scheduling policy as the OSEK metric apps.
 */

#ifndef FREERTOS_CONFIG_H_
#define FREERTOS_CONFIG_H_

/* QEMU virt: CLINT machine timer at 10 MHz, same tick as the OSEK metric apps */
#define configMTIME_BASE_ADDRESS                ( 0x0200BFF8UL )
#define configMTIMECMP_BASE_ADDRESS             ( 0x02004000UL )
#define configCPU_CLOCK_HZ                      ( 10000000UL )
#define configTICK_RATE_HZ                      ( 100 )
#define configTICK_TYPE_WIDTH_IN_BITS           TICK_TYPE_WIDTH_32_BITS

/* fixed priority preemptive without time slicing, as OSEK */
#define configUSE_PREEMPTION                    1
#define configUSE_TIME_SLICING                  0
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0
#define configMAX_PRIORITIES                    8
#define configIDLE_SHOULD_YIELD                 1

#define configMINIMAL_STACK_SIZE                ( 256 )
#define configISR_STACK_SIZE_WORDS              ( 512 )
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configSUPPORT_STATIC_ALLOCATION         0
#define configTOTAL_HEAP_SIZE                   ( 32 * 1024 )
#define configMAX_TASK_NAME_LEN                 ( 8 )

/* leave out everything the metric apps do not use, as the OSEK builds
 * have extended error checking off */
#define configUSE_TASK_NOTIFICATIONS            1
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configUSE_MUTEXES                       0
#define configUSE_RECURSIVE_MUTEXES             0
#define configUSE_COUNTING_SEMAPHORES           0
#define configUSE_QUEUE_SETS                    0
#define configUSE_TIMERS                        0
#define configUSE_CO_ROUTINES                   0
#define configUSE_TRACE_FACILITY                0
#define configUSE_STATS_FORMATTING_FUNCTIONS    0
#define configGENERATE_RUN_TIME_STATS           0
#define configCHECK_FOR_STACK_OVERFLOW          0
#define configQUEUE_REGISTRY_SIZE               0
#define configUSE_MALLOC_FAILED_HOOK            0

#define INCLUDE_vTaskDelay                      1
#define INCLUDE_vTaskDelete                     0
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_xTaskGetSchedulerState          0

#endif /* FREERTOS_CONFIG_H_ */
