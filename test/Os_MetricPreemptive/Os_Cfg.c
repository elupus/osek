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

unsigned char task0_stack[8192*16];
unsigned char task1_stack[8192*16];
unsigned char task2_stack[8192*16];
unsigned char task3_stack[8192*16];
unsigned char task4_stack[8192*16];
unsigned char task5_stack[8192*16];

unsigned int  task0_count;
unsigned int  task1_count;
unsigned int  task2_count;
unsigned int  task3_count;
unsigned int  task4_count;
unsigned int  task5_count;

void task0(void)
{
    task0_count++;
    Os_ActivateTask(1);
    Os_ChainTask(0);
}

void task1(void)
{
    task1_count++;
    Os_ActivateTask(2);
    Os_TerminateTask();
}

void task2(void)
{
    task2_count++;
    Os_ActivateTask(3);
    Os_TerminateTask();
}

void task3(void)
{
    task3_count++;
    Os_ActivateTask(4);
    Os_TerminateTask();
}

void task4(void)
{
    task4_count++;
    Os_TerminateTask();
}

void task5(void)
{
    task5_count++;
    if(task5_count == 1) {
        Os_SetRelAlarm(0, 1000u*1000u/OS_TICK_US+1, 0u);
        Os_ActivateTask(0);
        Os_TerminateTask();
    } else {
        Os_Shutdown();
    }
}

const Os_TaskConfigType Os_DefaultTasks[OS_TASK_COUNT] = {
          { NAMED_INIT(entry)       task0,
            NAMED_INIT(stack)       task0_stack,
            NAMED_INIT(stack_size)  sizeof(task0_stack),
            NAMED_INIT(autostart)   0,
            NAMED_INIT(priority)    0,
#if( (OS_CONFORMANCE == OS_CONFORMANCE_ECC2) ||  (OS_CONFORMANCE == OS_CONFORMANCE_BCC2) )
            NAMED_INIT(activation)  255u,
#endif
            NAMED_INIT(resource)    OS_INVALID_RESOURCE
          }
        , { NAMED_INIT(entry)       task1,
            NAMED_INIT(stack)       task1_stack,
            NAMED_INIT(stack_size)  sizeof(task1_stack),
            NAMED_INIT(autostart)   0,
            NAMED_INIT(priority)    1,
#if( (OS_CONFORMANCE == OS_CONFORMANCE_ECC2) ||  (OS_CONFORMANCE == OS_CONFORMANCE_BCC2) )
            NAMED_INIT(activation)  255u,
#endif
            NAMED_INIT(resource)    OS_INVALID_RESOURCE
          }
       , { NAMED_INIT(entry)        task2,
           NAMED_INIT(stack)        task2_stack,
           NAMED_INIT(stack_size)   sizeof(task2_stack),
           NAMED_INIT(autostart)    0,
           NAMED_INIT(priority)     2,
#if( (OS_CONFORMANCE == OS_CONFORMANCE_ECC2) ||  (OS_CONFORMANCE == OS_CONFORMANCE_BCC2) )
           NAMED_INIT(activation)   255u,
#endif
           NAMED_INIT(resource)     OS_INVALID_RESOURCE
         }
       , { NAMED_INIT(entry)        task3,
           NAMED_INIT(stack)        task3_stack,
           NAMED_INIT(stack_size)   sizeof(task3_stack),
           NAMED_INIT(autostart)    0,
           NAMED_INIT(priority)     3,
#if( (OS_CONFORMANCE == OS_CONFORMANCE_ECC2) ||  (OS_CONFORMANCE == OS_CONFORMANCE_BCC2) )
           NAMED_INIT(activation)   255u,
#endif
           NAMED_INIT(resource)     OS_INVALID_RESOURCE
         }
       , { NAMED_INIT(entry)        task4,
           NAMED_INIT(stack)        task4_stack,
           NAMED_INIT(stack_size)   sizeof(task4_stack),
           NAMED_INIT(autostart)    0,
           NAMED_INIT(priority)     4,
#if( (OS_CONFORMANCE == OS_CONFORMANCE_ECC2) ||  (OS_CONFORMANCE == OS_CONFORMANCE_BCC2) )
           NAMED_INIT(activation)   255u,
#endif
           NAMED_INIT(resource)     OS_INVALID_RESOURCE
         }
       , { NAMED_INIT(entry)        task5,
           NAMED_INIT(stack)        task5_stack,
           NAMED_INIT(stack_size)   sizeof(task5_stack),
           NAMED_INIT(autostart)    1,
           NAMED_INIT(priority)     5,
#if( (OS_CONFORMANCE == OS_CONFORMANCE_ECC2) ||  (OS_CONFORMANCE == OS_CONFORMANCE_BCC2) )
           NAMED_INIT(activation)   255u,
#endif
           NAMED_INIT(resource)     OS_INVALID_RESOURCE
         }
};

const Os_ResourceConfigType Os_DefaultResources[OS_RES_COUNT] = {
        {   NAMED_INIT(priority)  OS_PRIO_COUNT
        },
};

const Os_AlarmConfigType Os_DefaultAlarms[OS_ALARM_COUNT] = {
        {   NAMED_INIT(task)     5,
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
    unsigned int total = task0_count
                       + task1_count
                       + task2_count
                       + task3_count
                       + task4_count;
    printf("Execution counts %d (%d, %d, %d, %d, %d)\n"
            , total / 5
            , task0_count
            , task1_count
            , task2_count
            , task3_count
            , task4_count);
    return 0;
}



