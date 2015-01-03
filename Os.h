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

#ifndef OS_H_
#define OS_H_

#include "Std_Types.h"
#include "Os_Types.h"
#include "Os_Cfg.h"

typedef struct Os_ConfigType {
    const Os_TaskConfigType     (*tasks)[OS_TASK_COUNT];
    const Os_ResourceConfigType (*resources)[OS_RES_COUNT];
} Os_ConfigType;


extern Os_TaskControlType              Os_TaskControls        [OS_TASK_COUNT];
extern Os_ReadyListType                Os_TaskReady           [OS_PRIO_COUNT];
extern Os_TaskType                     Os_TaskRunning;
extern Os_ContextType                  Os_CallContext;
extern const Os_TaskConfigType *       Os_TaskConfigs;

void       Os_Init(const Os_ConfigType* config);
void       Os_Start(void);
void       Os_Isr(void);
StatusType Os_Schedule(void);
StatusType Os_TerminateTask(void);
StatusType Os_ActivateTask(Os_TaskType task);

StatusType Os_GetResource(Os_ResourceType res);
StatusType Os_ReleaseResource(Os_ResourceType res);

static __inline StatusType Os_GetTaskId   (Os_TaskType* task)
{
    *task = Os_TaskRunning;
    return E_OK;
}

#ifdef OS_ERRORHOOK
#define OS_ERRORCHECK(_condition, _ret) do { \
        if(!(_condition)) {                  \
            OS_ERRORHOOK(_ret);              \
        }                                    \
    } while(0)
#else
#define OS_ERRORCHECK(_condition, _ret)
#endif

#endif /* OS_H_ */
