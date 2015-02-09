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

unsigned char idle_stack[8192*16];
unsigned char busy_stack[8192*16];
unsigned char test_stack[8192*16];
unsigned char test2_stack[8192*16];

void idle(void)
{
    Os_SetRelAlarm(0, 5, 5);
    Os_SetRelAlarm(1, 15, 5);
    Os_SetRelAlarm(2, 10, 5);

    Os_CancelAlarm(2);
    while(1) {
        Os_ActivateTask(1);
    }
    Os_TerminateTask();
}

void busy(void)
{
    int i;
    for(i = 0; i < 10000; i++) {
        /* NOP */
    }
    Os_GetResource(OS_RES_SCHEDULER);
    Os_ActivateTask(2);
    for(i = 0; i < 10000; i++) {
        /* NOP */
    }
    Os_ReleaseResource(OS_RES_SCHEDULER);
    for(i = 0; i < 10000; i++) {
        /* NOP */
    }

    Os_TerminateTask();
}

void test(void)
{
    Os_TerminateTask();
}

void test2(void)
{
    Os_TerminateTask();
}

const Os_TaskConfigType Os_DefaultTasks[OS_TASK_COUNT] = {
          { .entry      = idle,
            .stack      = idle_stack,
            .stack_size = sizeof(idle_stack),
            .autostart  = 1,
            .priority   = 0,
#if( (OS_CONFORMANCE == OS_CONFORMANCE_ECC2) ||  (OS_CONFORMANCE == OS_CONFORMANCE_BCC2) )

            .activation = 255u,
#endif
            .resource   = OS_INVALID_RESOURCE
          }
        , { .entry      = busy,
            .stack      = busy_stack,
            .stack_size = sizeof(busy_stack),
            .autostart  = 0,
            .priority   = 1,
#if( (OS_CONFORMANCE == OS_CONFORMANCE_ECC2) ||  (OS_CONFORMANCE == OS_CONFORMANCE_BCC2) )
            .activation = 255u,
#endif
            .resource   = OS_INVALID_RESOURCE
          }
       , { .entry       = test,
           .stack       = test_stack,
           .stack_size  = sizeof(test_stack),
           .autostart   = 0,
           .priority    = 2,
#if( (OS_CONFORMANCE == OS_CONFORMANCE_ECC2) ||  (OS_CONFORMANCE == OS_CONFORMANCE_BCC2) )
           .activation  = 255u,
#endif
           .resource    = 3
         }
       , { .entry       = test2,
           .stack       = test2_stack,
           .stack_size  = sizeof(test_stack),
           .autostart   = 0,
           .priority    = 2,
#if( (OS_CONFORMANCE == OS_CONFORMANCE_ECC2) ||  (OS_CONFORMANCE == OS_CONFORMANCE_BCC2) )
           .activation  = 255u,
#endif
           .resource    = 3
         }
};

const Os_ResourceConfigType Os_DefaultResources[OS_RES_COUNT] = {
        {   .priority = OS_PRIO_COUNT
        },
        {   .priority = 2
        },
        {   .priority = 1
        },
        {   .priority = 2
        },
};

const Os_AlarmConfigType Os_DefaultAlarms[OS_ALARM_COUNT] = {
        {   .task    = 3,
            .counter = OS_COUNTER_SYSTEM
        },
        {   .task  = 3,
            .counter = OS_COUNTER_SYSTEM
        },
        {   .task  = 3,
            .counter = OS_COUNTER_SYSTEM
        },
};

void Os_PreTaskHook(Os_TaskType task)
{
    printf("PRETASK %d\n", task);
}

void Os_PostTaskHook(Os_TaskType task)
{
    printf("POSTTASK %d\n", task);
}

void Os_ErrorHook(Os_StatusType ret)
{
    exit(ret);
}

const Os_ConfigType Os_DefaultConfig = {
        .tasks     = &Os_DefaultTasks,
        .resources = &Os_DefaultResources,
        .alarms    = &Os_DefaultAlarms,
};

int main(void)
{
    Os_Init(&Os_DefaultConfig);
    Os_Start(); /* NO RETURN */
    return 0;
}



