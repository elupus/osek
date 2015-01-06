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

void       Os_TaskInternalResource_Release(void);
void       Os_TaskInternalResource_Get(void);

void       Os_Init(const Os_ConfigType* config);
void       Os_Start(void);
void       Os_Isr(void);

extern          StatusType Os_Schedule_Internal(void);
static __inline StatusType Os_Schedule         (void)
{
    StatusType result;
    Os_Arch_DisableAllInterrupts();

    Os_TaskInternalResource_Release();

    result = Os_Schedule_Internal();

    Os_TaskInternalResource_Get();

    Os_Arch_EnableAllInterrupts();
    return result;
}

extern StatusType          Os_TerminateTask_Internal(void);
static __inline StatusType Os_TerminateTask         (void)
{
    StatusType result;
    Os_Arch_DisableAllInterrupts();

    Os_TaskInternalResource_Release();

    result = Os_TerminateTask_Internal();

    Os_TaskInternalResource_Get();

    Os_Arch_EnableAllInterrupts();
    return result;
}

extern StatusType          Os_ActivateTask_Internal(Os_TaskType task);
static __inline StatusType Os_ActivateTask         (Os_TaskType task)
{
    StatusType result;
    Os_Arch_DisableAllInterrupts();
    result = Os_ActivateTask_Internal(task);
    Os_Arch_EnableAllInterrupts();
    return result;
}


extern          StatusType Os_GetResource_Internal(Os_ResourceType res);
static __inline StatusType Os_GetResource         (Os_ResourceType res)
{
    StatusType result;
    Os_Arch_DisableAllInterrupts();
    result = Os_GetResource_Internal(res);
    Os_Arch_EnableAllInterrupts();
    return result;
}

extern          StatusType Os_ReleaseResource_Internal(Os_ResourceType res);
static __inline StatusType Os_ReleaseResource         (Os_ResourceType res)
{
    StatusType result;
    Os_Arch_DisableAllInterrupts();
    result = Os_ReleaseResource_Internal(res);
    Os_Arch_EnableAllInterrupts();
    return result;
}

/**
 * @brief Get the identifier of the currently executing task
 * @param[out] task Currently running task or Os_TaskIdNone if no task is running
 * @return E_OK on success
 */
static __inline StatusType Os_GetTaskId   (Os_TaskType* task)
{
    *task = Os_TaskRunning;
    return E_OK;
}

#if(OS_PRETASKHOOK_ENABLE)
extern void Os_PreTaskHook(Os_TaskType task);
#define OS_PRETASKHOOK(task) Os_PreTaskHook(task)
#else
#define OS_PRETASKHOOK(task)
#endif

#if(OS_POSTTASKHOOK_ENABLE)
extern void Os_PostTaskHook(Os_TaskType task);
#define OS_POSTTASKHOOK(task) Os_PostTaskHook(task)
#else
#define OS_POSTTASKHOOK(task)
#endif

#if(OS_ERRORHOOK_ENABLE)
extern void Os_ErrorHook(StatusType status);
#define OS_ERRORHOOK(status) Os_ErrorHook(status)
#else
#define OS_ERRORHOOK(status)
#endif

#if(OS_ERRORHOOK_ENABLE)
#define OS_ERRORCHECK(_condition, _ret) do { \
        if(!(_condition)) {                  \
            OS_ERRORHOOK(_ret);              \
        }                                    \
    } while(0)

#define OS_ERRORCHECK_R(_condition, _ret) do { \
        if(!(_condition)) {                    \
            OS_ERRORHOOK(_ret);                \
            return _ret;                       \
        }                                      \
    } while(0)
#else
#define OS_ERRORCHECK(_condition, _ret)
#endif

#endif /* OS_H_ */
