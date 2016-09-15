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
 * @ingroup Os
 */

#ifndef OS_H_
#define OS_H_

#include "Std_Types.h"
#include "Os_Types.h"
#include "Os_Cfg.h"

#ifdef OS_CFG_ARCH_POSIX
#include "Os_Arch_Posix.h"
#endif

#ifdef OS_CFG_ARCH_FIBERS
#include "Os_Arch_Fibers.h"
#endif

#ifdef OS_CFG_ARCH_HCS12
#include "Os_Arch_HCS12.h"
#endif

#ifndef OS_ERROR_EXT_ENABLE
#define OS_ERROR_EXT_ENABLE 1
#endif

#ifndef OS_CONFORMANCE
#define OS_CONFORMANCE OS_CONFORMANCE_ECC2
#endif

#ifndef OS_COUNTER_COUNT
#define OS_COUNTER_COUNT  (Os_CounterType)1u
#define OS_COUNTER_SYSTEM (Os_CounterType)0u
#endif

#ifdef __GNUC__
#define Os_Unlikely(x)  __builtin_expect((x),0)
#define Os_Likely(x)    __builtin_expect((x),1)
#else
#define Os_Unlikely(x) (x)
#define Os_Likely(x)   (x)
#endif

/**
 * @brief internal structure to keep track of state on syscalls
 */
typedef struct Os_SyscallState {
    Os_IrqState   irq;
    Os_TaskType   task_before;
} Os_SyscallStateType;

/**
 * @brief Structure describing a tasks static configuration
 */
typedef struct Os_TaskConfigType {
    Os_PriorityType  priority;    /**< @brief fixed priority of task */
    Os_TaskEntryType entry;       /**< @brief entry point of task */
    void*            stack;       /**< @brief bottom of stack pointer */
    size_t           stack_size;  /**< @brief how large is the stack pointed to by stack */
    boolean          autostart;   /**< @brief should this task start automatically */
#if( (OS_CONFORMANCE == OS_CONFORMANCE_ECC2) ||  (OS_CONFORMANCE == OS_CONFORMANCE_BCC2) )
    uint8            activation;  /**< @brief maximum number of activations allowed */
#endif
    Os_ResourceType  resource;    /**< @brief internal resource of task, can be Os_TaskIdNone */
} Os_TaskConfigType;

/**
 * @brief Structure holding information of the state of a task
 */
typedef struct Os_TaskControlType {
    Os_TaskStateEnum state;       /**< @brief current state */
#if( (OS_CONFORMANCE == OS_CONFORMANCE_ECC2) ||  (OS_CONFORMANCE == OS_CONFORMANCE_BCC2) )
    uint8            activation;  /**< @brief number of activations for given task */
#endif
    Os_TaskType      next;        /**< @brief next task in the same ready list */
    Os_ResourceType  resource;    /**< @brief last taken resource for task (rest is linked list */
    Os_PriorityType  priority;
} Os_TaskControlType;

/**
 * @brief Structure holding configuration setup for each resource
 */
typedef struct Os_ResourceConfigType {
    Os_PriorityType priority;     /**< @brief priority ceiling of all task using */
} Os_ResourceConfigType;

/**
 * @brief Structure holding active state information for each resource
 */
typedef struct Os_ResourceControlType {
    Os_ResourceType next;         /**< @brief linked list of held resources */
#if(OS_ERROR_EXT_ENABLE)
    Os_TaskType     task;         /**< @brief task currently holding resource */
#endif
} Os_ResourceControlType;

/**
 * @brief Structure holding configuration setup for each alarm
 */
typedef struct Os_AlarmConfigType {
    Os_TaskType     task;         /**< @brief task to activate */
    Os_CounterType  counter;      /**< @brief counter driving this alarm */
} Os_AlarmConfigType;

typedef uint8 Os_AlarmQueueIndexType;

typedef struct Os_CounterControlType {
    Os_TickType     ticks;
    Os_AlarmType    queue[OS_ALARM_COUNT+1]; /**< 1 based binary heap, [0] contain number of active entries */
} Os_CounterControlType;

/**
 * @brief Linked list of ready tasks
 */
typedef struct Os_ReadyListType {
    Os_TaskType head;             /**< @brief pointer to the first ready task */
    Os_TaskType tail;             /**< @brief pointer to the last ready task */
} Os_ReadyListType;

/**
 * @brief Main configuration structure of Os
 */
typedef struct Os_ConfigType {
    const Os_TaskConfigType     (*tasks)[OS_TASK_COUNT];    /**< @brief pointer to an array of task configurations */
    const Os_ResourceConfigType (*resources)[OS_RES_COUNT]; /**< @brief pointer to an array of resource configurations */
#ifdef OS_ALARM_COUNT
    const Os_AlarmConfigType    (*alarms)[OS_ALARM_COUNT];  /**< @brief pointer to an array of alarm configurations */
#endif
} Os_ConfigType;

typedef uint8 Os_ServiceType;

typedef struct Os_ErrorType {
    Os_ServiceType service;
    Os_StatusType  status;
#if(OS_ERROR_EXT_ENABLE)
    uint16         line;
#endif
    uint16         params[3];
} Os_ErrorType;

extern Os_ErrorType                    Os_Error;
extern Os_TaskControlType              Os_TaskControls        [OS_TASK_COUNT];
extern Os_ReadyListType                Os_TaskReady           [OS_PRIO_COUNT];
extern Os_TaskType                     Os_ActiveTask;
extern Os_ContextType                  Os_CallContext;
extern const Os_TaskConfigType *       Os_TaskConfigs;

void       Os_TaskInternalResource_Release(void);
void       Os_TaskInternalResource_Get(void);

void       Os_Init(const Os_ConfigType* config);
void       Os_Start(void);
void       Os_Shutdown(void);
void       Os_Isr(void);

extern Os_StatusType Os_Schedule(void);
extern Os_StatusType Os_TerminateTask(void);
extern Os_StatusType Os_ActivateTask(Os_TaskType task);
extern Os_StatusType Os_ChainTask(Os_TaskType task);
extern Os_StatusType Os_GetResource(Os_ResourceType res);
extern Os_StatusType Os_ReleaseResource(Os_ResourceType res);

extern Os_StatusType Os_SetRelAlarm(Os_AlarmType alarm, Os_TickType increment, Os_TickType cycle);
extern Os_StatusType Os_SetAbsAlarm(Os_AlarmType alarm, Os_TickType start    , Os_TickType cycle);
extern Os_StatusType Os_CancelAlarm(Os_AlarmType alarm);
extern Os_StatusType Os_GetAlarm   (Os_AlarmType alarm, Os_TickType* tick);

extern Os_StatusType Os_IncrementCounter(Os_CounterType counter);

/**
 * @brief Get the identifier of the currently executing task
 * @param[out] task Currently running task or Os_TaskIdNone if no task is running
 * @return E_OK on success
 */
static __inline Os_StatusType Os_GetTaskId   (Os_TaskType* task)
{
    if (Os_TaskControls[Os_ActiveTask].state == OS_TASK_RUNNING) {
        *task = Os_ActiveTask;
    } else {
        *task = OS_INVALID_TASK;
    }
    return E_OK;
}

enum {
    OSServiceId_None,
    OSServiceId_Schedule,
    OSServiceId_TerminateTask,
    OSServiceId_ActivateTask,
    OSServiceId_GetResource,
    OSServiceId_ReleaseResource,
    OSServiceId_SetRelAlarm,
    OSServiceId_SetAbsAlarm,
    OSServiceId_CancelAlarm,
    OSServiceId_GetAlarm,
    OSServiceId_ChainTask,
    OSServiceId_CounterIncrement,
};

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
extern void Os_ErrorHook(Os_StatusType status);
#define OS_ERRORHOOK(status) Os_ErrorHook(status)
#else
#define OS_ERRORHOOK(status)
#endif

#if(OS_ERROR_EXT_ENABLE)
#define OS_ERRORCHECK_DATA(_ret)  \
	Os_Error.status  = _ret;      \
    Os_Error.line    = __LINE__;
#else
#define OS_ERRORCHECK_DATA(_ret)  \
	Os_Error.status  = _ret;
#endif

#define OS_ERRORCHECK(_condition, _ret)   do {   \
        if(Os_Unlikely(!(_condition))) {         \
            Os_Error.service = OSServiceId_None; \
            OS_ERRORCHECK_DATA(_ret)             \
            OS_ERRORHOOK(_ret);                  \
            return;                              \
        }                                        \
    } while(0)

#define OS_ERRORCHECK_R(_condition, _ret) do {   \
        if(Os_Unlikely(!(_condition))) {         \
            OS_ERRORCHECK_DATA(_ret)             \
            goto OS_ERRORCHECK_EXIT_POINT;       \
        }                                        \
    } while(0)

#define OS_CHECK(_condition, _ret)   OS_ERRORCHECK(_condition, _ret)
#define OS_CHECK_R(_condition, _ret) OS_ERRORCHECK_R(_condition, _ret)

#if(OS_ERROR_EXT_ENABLE)
#define OS_CHECK_EXT(_condition, _ret)   OS_ERRORCHECK(_condition, _ret)
#define OS_CHECK_EXT_R(_condition, _ret) OS_ERRORCHECK_R(_condition, _ret)
#else
#define OS_CHECK_EXT(_condition, _ret)
#define OS_CHECK_EXT_R(_condition, _ret)
#endif


#endif /* OS_H_ */
