/* OSEKOS Implementation of an OSEK Scheduler
 * Copyright (C) 2015 Joakim Plate
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
 * @ingroup Os_Cfg
 *
 * Event ping-pong: a low priority task sets an event that a higher priority
 * extended task waits for, which then clears it and waits again. Each round
 * is a wake up and a block, two task switches without any task restart, the
 * same as a task notification ping-pong in other kernels.
 */

#include "Std_Types.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "Os.h"
#include "Os_Cfg.h"
#include "Os_Types.h"

#define NAMED_INIT(a) .a =

#define TASK_LO   (Os_TaskType)0
#define TASK_HI   (Os_TaskType)1
#define TASK_SUP  (Os_TaskType)2
#define EVENT_GO  (Os_EventMaskType)0x1u

unsigned char task_lo_stack [OS_STACK_SIZE(512)];
unsigned char task_hi_stack [OS_STACK_SIZE(512)];
unsigned char task_sup_stack[OS_STACK_SIZE(512)];

volatile unsigned int task_lo_count;
volatile unsigned int task_hi_count;
volatile unsigned int task_sup_count;

void task_lo(void)
{
    while (1) {
        task_lo_count++;
        Os_SetEvent(TASK_HI, EVENT_GO);
    }
}

void task_hi(void)
{
    while (1) {
        Os_WaitEvent(EVENT_GO);
        Os_ClearEvent(EVENT_GO);
        task_hi_count++;
    }
}

void task_sup(void)
{
    task_sup_count++;
    if (task_sup_count == 1) {
        Os_SetRelAlarm(0, (1000ul*1000ul/OS_TICK_US)+1ul, 0u);
        Os_ActivateTask(TASK_HI);
        Os_ActivateTask(TASK_LO);
        Os_TerminateTask();
    } else {
        Os_Shutdown(E_OK);
    }
}

const Os_TaskConfigType Os_DefaultTasks[OS_TASK_COUNT] = {
          { NAMED_INIT(priority)    0,
            NAMED_INIT(entry)       task_lo,
            NAMED_INIT(stack)       task_lo_stack,
            NAMED_INIT(stack_size)  sizeof(task_lo_stack),
            NAMED_INIT(autostart)   0,
            NAMED_INIT(resource)    OS_INVALID_RESOURCE,
            NAMED_INIT(extended)    FALSE
          }
        , { NAMED_INIT(priority)    1,
            NAMED_INIT(entry)       task_hi,
            NAMED_INIT(stack)       task_hi_stack,
            NAMED_INIT(stack_size)  sizeof(task_hi_stack),
            NAMED_INIT(autostart)   0,
            NAMED_INIT(resource)    OS_INVALID_RESOURCE,
            NAMED_INIT(extended)    TRUE
          }
        , { NAMED_INIT(priority)    2,
            NAMED_INIT(entry)       task_sup,
            NAMED_INIT(stack)       task_sup_stack,
            NAMED_INIT(stack_size)  sizeof(task_sup_stack),
            NAMED_INIT(autostart)   1,
            NAMED_INIT(resource)    OS_INVALID_RESOURCE,
            NAMED_INIT(extended)    FALSE
          }
};

const Os_ResourceConfigType Os_DefaultResources[OS_RES_COUNT] = {
        {   NAMED_INIT(priority)  OS_PRIO_COUNT
        },
};

const Os_AlarmConfigType Os_DefaultAlarms[OS_ALARM_COUNT] = {
        {   NAMED_INIT(task)     TASK_SUP,
            NAMED_INIT(counter)  OS_COUNTER_SYSTEM
        },
};

const Os_ConfigType Os_DefaultConfig = {
        NAMED_INIT(tasks)      &Os_DefaultTasks,
        NAMED_INIT(resources)  &Os_DefaultResources,
        NAMED_INIT(alarms)     &Os_DefaultAlarms,
};

int main(void)
{
    Os_Init(&Os_DefaultConfig);
    Os_Start(OS_DEFAULTAPPMODE);
    printf("Execution counts %u (%u, %u)\n"
            , task_hi_count
            , task_lo_count
            , task_hi_count);
    return 0;
}
