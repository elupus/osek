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

unsigned char task0_stack[1024];
unsigned char task1_stack[1024];

void task0(void)
{
    while(1) {
        Os_ActivateTask(1);
    }
    Os_TerminateTask();
}

void task1(void)
{
    Os_TerminateTask();
}


const Os_TaskConfigType Os_DefaultTasks[OS_TASK_COUNT] = {
          { .entry      = task0,
            .stack      = task0_stack,
            .stack_size = sizeof(task0_stack),
            .autostart  = 1,
            .priority   = 0,
            .resource   = OS_INVALID_RESOURCE
          }
        , { .entry      = task1,
            .stack      = task1_stack,
            .stack_size = sizeof(task1_stack),
            .autostart  = 0,
            .priority   = 1,
            .resource   = OS_INVALID_RESOURCE
          }
};

const Os_ResourceConfigType Os_DefaultResources[OS_RES_COUNT] = {
        {   .priority = OS_PRIO_COUNT
        },
};

const Os_AlarmConfigType Os_DefaultAlarms[OS_ALARM_COUNT] = {
        {   .task  = 0 },
};

void Os_PreTaskHook(Os_TaskType task)
{
}

void Os_PostTaskHook(Os_TaskType task)
{
}

void Os_ErrorHook(Os_StatusType ret)
{
    while(1) {
        ; /* NOP */
    }
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



