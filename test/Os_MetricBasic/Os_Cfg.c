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
 */

#include "Std_Types.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "Os.h"
#include "Os_Cfg.h"
#include "Os_Types.h"

#ifdef __HIWARE__
#define NAMED_INIT(a)
#else
#define NAMED_INIT(a) .a =
#endif

unsigned char task0_stack[512];
unsigned char task1_stack[512];

unsigned int  task0_count;
unsigned int  task1_count;
unsigned int  task0_array[1024];

void task0(void)
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

void task1(void)
{
    task1_count++;
    if(task1_count == 1) {
        Os_SetRelAlarm(0, (1000ul*1000ul/OS_TICK_US)+1ul, 0u);
        Os_ActivateTask(0);
        Os_TerminateTask();
    } else {
        Os_Shutdown();
    }
}

const Os_TaskConfigType Os_DefaultTasks[OS_TASK_COUNT] = {
          { NAMED_INIT(priority)    0,
            NAMED_INIT(entry)       task0,
            NAMED_INIT(stack)       task0_stack,
            NAMED_INIT(stack_size)  sizeof(task0_stack),
            NAMED_INIT(autostart)   0,
#if( (OS_CONFORMANCE == OS_CONFORMANCE_ECC2) ||  (OS_CONFORMANCE == OS_CONFORMANCE_BCC2) )
            NAMED_INIT(activation)  255u,
#endif
            NAMED_INIT(resource)    OS_INVALID_RESOURCE
          }
        , { NAMED_INIT(priority)    1,
            NAMED_INIT(entry)       task1,
            NAMED_INIT(stack)       task1_stack,
            NAMED_INIT(stack_size)  sizeof(task1_stack),
            NAMED_INIT(autostart)   1,
#if( (OS_CONFORMANCE == OS_CONFORMANCE_ECC2) ||  (OS_CONFORMANCE == OS_CONFORMANCE_BCC2) )
            NAMED_INIT(activation)  255u,
#endif
            NAMED_INIT(resource)    OS_INVALID_RESOURCE
          }
};

const Os_ResourceConfigType Os_DefaultResources[OS_RES_COUNT] = {
        {   NAMED_INIT(priority)  OS_PRIO_COUNT
        },
};

const Os_AlarmConfigType Os_DefaultAlarms[OS_ALARM_COUNT] = {
        {   NAMED_INIT(task)     1,
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
    Os_Start();
    printf("Execution counts %u\n"
            , task0_count);
    return 0;
}



