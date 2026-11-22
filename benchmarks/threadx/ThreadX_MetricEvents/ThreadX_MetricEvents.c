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
 * ThreadX version of Os_MetricEvents: an event flags ping-pong. Each round
 * is a wake up and a block, two thread switches without any thread
 * restart, like the OSEK event ping-pong.
 */

#include "ThreadX_Metric.h"

/* task_lo (prio 2) sets the flag task_hi (prio 1) waits for; task_hi gets
 * and clears it and waits again. ThreadX priorities are reversed, 0 is the
 * highest. */
volatile unsigned int task_lo_count, task_hi_count;
static ThreadX_Metric_ThreadType task_lo_thread, task_hi_thread;
static TX_EVENT_FLAGS_GROUP task_hi_events;

#define TASK_HI_EVENT 1u

static void task_lo(ULONG arg)
{
    while (1) {
        task_lo_count++;
        tx_event_flags_set(&task_hi_events, TASK_HI_EVENT, TX_OR);
    }
}

static void task_hi(ULONG arg)
{
    ULONG actual;
    while (1) {
        tx_event_flags_get(&task_hi_events, TASK_HI_EVENT, TX_OR_CLEAR,
                           &actual, TX_WAIT_FOREVER);
        task_hi_count++;
    }
}

static void task_sup(ULONG arg)
{
    tx_event_flags_create(&task_hi_events, "hi");
    ThreadX_Metric_CreateThread(&task_hi_thread, task_hi, 0, 1);
    ThreadX_Metric_CreateThread(&task_lo_thread, task_lo, 0, 2);
    THREADX_METRIC_RUN_ONE_SECOND();
    printf("Execution counts %u (%u, %u)\n", task_hi_count, task_lo_count, task_hi_count);
    exit(0);
}

int main(void) { ThreadX_Metric_Start(); }
