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

/**
 * @defgroup Os OSEK OS Scheduler
 */

/**
 * @defgroup Os_Cfg OSEK Os Scheduler Config
 * @ingroup Os
 */

#include <string.h>

#include "Std_Types.h"
#include "Os_Types.h"
#include "Os.h"

Os_ErrorType                    Os_Error;
Os_TaskControlType              Os_TaskControls        [OS_TASK_COUNT]; /**< control array for tasks */
Os_ReadyListType                Os_TaskReady           [OS_PRIO_COUNT]; /**< array of ready lists based on priority */
Os_TaskType                     Os_TaskRunning;                         /**< currently running task */
Os_ContextType                  Os_CallContext;                         /**< current call context */
const Os_TaskConfigType *       Os_TaskConfigs;                         /**< config array for tasks */

const Os_ResourceConfigType *   Os_ResourceConfigs;                     /**< config array for resources */
Os_ResourceControlType          Os_ResourceControls    [OS_RES_COUNT];  /**< control array for resources */

volatile Os_TickType            Os_Ticks;                               /**< os ticks that have elapsed */
volatile boolean                Os_Continue;                            /**< should starting task continue */


#ifdef OS_ALARM_COUNT
Os_AlarmControlType             Os_AlarmControls       [OS_ALARM_COUNT]; /**< control array for alarms */
const Os_AlarmConfigType *      Os_AlarmConfigs;                         /**< config array for alarms  */
Os_AlarmType                    Os_AlarmNext;
#endif

static Os_StatusType Os_Schedule_Internal(void);
static Os_StatusType Os_TerminateTask_Internal(void);
static Os_StatusType Os_ActivateTask_Internal(Os_TaskType task);
static Os_StatusType Os_GetResource_Task(Os_ResourceType res);
static Os_StatusType Os_ReleaseResource_Task(Os_ResourceType res);

static void Os_AlarmTrigger(Os_AlarmType alarm);
static void Os_AlarmTick(void);
static void Os_AlarmAdd(Os_AlarmType alarm, Os_TickType delay);

/**
 * @brief Add task to the given ready list at the head of the list
 * @param[in] list ready list to add task to
 * @param[in] task task to add to list
 */
static void Os_ReadyListPushHead(Os_ReadyListType* list, Os_TaskType task)
{
    Os_TaskControls[task].next = list->head;
    list->head = task;
    if (list->tail == OS_INVALID_TASK) {
        list->tail = task;
    }
}

/**
 * @brief Pop task from the head of the ready list
 * @param[in]  list ready list to add task to
 * @param[out] task popped from list, Os_TaskIdNone if nothing available.
 */
static void Os_ReadyListPopHead(Os_ReadyListType* list, Os_TaskType* task)
{
    if (list->tail == OS_INVALID_TASK) {
        *task = OS_INVALID_TASK;
    } else {
        *task = list->head;
        if (list->tail == *task) {
            list->tail = OS_INVALID_TASK;
            list->head = OS_INVALID_TASK;
        }
        list->head = Os_TaskControls[*task].next;
    }
    Os_TaskControls[*task].next = OS_INVALID_TASK;
}

/**
 * @brief Add task to given ready list at the tail of the list
 * @param[in] list ready list to add task to
 * @param[in] task task to add to list
 */
static void Os_ReadyListPushTail(Os_ReadyListType* list, Os_TaskType task)
{
    Os_TaskControls[task].next = OS_INVALID_TASK;
    if (list->head == OS_INVALID_TASK) {
        list->head = task;
    } else {
        Os_TaskControls[list->tail].next = task;
    }
    list->tail = task;
}

/**
 * @brief Initializes a ready list as empty
 * @param[in] list the list to initialize
 */
static void Os_ReadyListInit(Os_ReadyListType* list)
{
    list->head = OS_INVALID_TASK;
    list->tail = OS_INVALID_TASK;
}

/**
 * @brief Execute the configured action of an alarm
 * @param alarm
 */
void Os_AlarmTrigger(Os_AlarmType alarm)
{
    /* TODO */
    if (Os_AlarmConfigs[alarm].task != OS_INVALID_TASK) {
        /* TODO event */
        (void)Os_ActivateTask_Internal(Os_AlarmConfigs[alarm].task);
    }

    if (Os_AlarmControls[alarm].cycle) {
        Os_AlarmAdd(alarm, Os_AlarmControls[alarm].cycle);
    }
}

void Os_AlarmTick(void)
{
    /* trigger and consume any expired */
    while (Os_AlarmNext != OS_INVALID_ALARM && Os_AlarmControls[Os_AlarmNext].ticks == 0u) {
        Os_AlarmType alarm = Os_AlarmNext;
        Os_AlarmNext = Os_AlarmControls[Os_AlarmNext].next;
        Os_AlarmTrigger(alarm);
    }
    /* decrement last active */
    if (Os_AlarmNext != OS_INVALID_ALARM) {
        Os_AlarmControls[Os_AlarmNext].ticks--;
    }
}

void Os_AlarmAdd(Os_AlarmType alarm, Os_TickType delay)
{
    Os_AlarmType index = Os_AlarmNext
               , prev  = OS_INVALID_ALARM;

    while (index != OS_INVALID_ALARM && Os_AlarmControls[index].ticks <= delay) {
        prev   = index;
        delay -= Os_AlarmControls[index].ticks;
        index  = Os_AlarmControls[index].next;
    }

    Os_AlarmControls[alarm].ticks = delay;
    Os_AlarmControls[alarm].next  = index;

    /* reduce next delay */
    if (index != OS_INVALID_ALARM) {
        Os_AlarmControls[index].ticks -= delay;
    }

    if (prev == OS_INVALID_ALARM) {
        /* empty list or first in list */
        Os_AlarmNext = alarm;
    } else {
        /* insert in list */
        Os_AlarmControls[prev ].next = alarm;
    }
}

/**
 * @brief Initializes task controls based on config
 * @param[in] task the task to initialize
 */
static void Os_TaskInit(Os_TaskType task)
{
    memset(&Os_TaskControls[task], 0, sizeof(Os_TaskControls[task]));
    Os_TaskControls[task].next     = OS_INVALID_TASK;
    Os_TaskControls[task].resource = OS_INVALID_RESOURCE;
}

/**
 * @brief Peeks into the ready lists for a task with higher or equal priority to given
 * @param[in]  min_priority minimal task priority to find
 * @param[out] task the found task. will be Os_TaskIdNone if none was found
 */
static void Os_TaskPeek(Os_PriorityType min_priority, Os_TaskType* task)
{
    Os_PriorityType prio;
    *task = OS_INVALID_TASK;
    for(prio = OS_PRIO_COUNT; prio > min_priority && *task == OS_INVALID_TASK; --prio) {
        *task = Os_TaskReady[prio-1].head;
    }
}

/**
 * @brief  Get the current priority of a given task
 * @param  task the task to get the priority for
 * @return priority of the task
 *
 * This will return the priority ceiling of the supplied
 * task and all the resources it is currently holding.
 */
static __inline Os_PriorityType Os_TaskPrio(Os_TaskType task)
{
    Os_PriorityType prio;
    Os_ResourceType resource = Os_TaskControls[task].resource;
    if (resource == OS_INVALID_TASK) {
        prio = Os_TaskConfigs[task].priority;
    } else {
        prio = Os_ResourceConfigs[resource].priority;
    }
    return prio;
}

/**
 * @brief Initializes a resource structure
 * @param res resource to initialize
 */
static void Os_ResourceInit(Os_ResourceType res)
{
    memset(&Os_ResourceControls[res], 0, sizeof(Os_ResourceControls[res]));
    Os_ResourceControls[res].next = OS_INVALID_RESOURCE;
    Os_ResourceControls[res].task = OS_INVALID_TASK;
}

/**
 * @brief Release the internal resource of running task
 *
 * If running task has an associated internal resource,
 * this will be released by call to this function.
 *
 * Can only be called from task context
 */
void Os_TaskInternalResource_Release(void)
{
    Os_ResourceType res;

    OS_CHECK_EXT(Os_CallContext == OS_CONTEXT_TASK, E_OS_STATE);
    OS_CHECK_EXT(Os_TaskRunning != OS_INVALID_TASK, E_OS_STATE);

    res = Os_TaskConfigs[Os_TaskRunning].resource;
    if (res != OS_INVALID_RESOURCE) {
        (void)Os_ReleaseResource_Task(res);
    }
}

/**
 * @brief Get the internal resource of running task
 *
 * If the running (or to be running) task has an associated internal
 * resource, this function will make sure the task holds that resource.
 *
 */
void Os_TaskInternalResource_Get(void)
{
    Os_ResourceType res;

    OS_CHECK_EXT(Os_TaskRunning != OS_INVALID_TASK  , E_OS_STATE);

    res = Os_TaskConfigs[Os_TaskRunning].resource;
    if (res != OS_INVALID_RESOURCE) {
        if (Os_ResourceControls[res].task != Os_TaskRunning) {
            (void)Os_GetResource_Task(res);
        }
    }
}

/**
 * @brief Perform the state transition from running to suspended for a task
 * @param task task to transition to suspended state
 */
static __inline void Os_State_Running_To_Suspended(Os_TaskType task)
{
    OS_CHECK_EXT(Os_TaskControls[task].state == OS_TASK_RUNNING, E_OS_STATE);

    Os_TaskControls[task].state = OS_TASK_SUSPENDED;

    OS_POSTTASKHOOK(task);
}

/**
 * @brief Perform the state transition from running back to ready
 * @param task task to transition to suspended state
 *
 * This will push the task back into the ready list at the top
 * of the list so it's next in line to execute again
 */
static __inline void Os_State_Running_To_Ready(Os_TaskType task)
{
    OS_CHECK_EXT(Os_TaskControls[task].state == OS_TASK_RUNNING, E_OS_STATE);

    Os_PriorityType prio = Os_TaskPrio(task);
    Os_ReadyListPushHead(&Os_TaskReady[prio], task);
    Os_TaskControls[task].state = OS_TASK_READY;
    Os_TaskRunning = OS_INVALID_TASK;

    OS_POSTTASKHOOK(task);
}

/**
 * @brief Perform the state transition from suspended to the ready list
 * @param task task to transition to the ready state
 *
 * This will prepare the stack structure for execution of this task's
 * main function and push the task into the tail of the ready list
 */
static __inline void Os_State_Suspended_To_Ready(Os_TaskType task)
{
    OS_CHECK_EXT(Os_TaskControls[task].state == OS_TASK_SUSPENDED, E_OS_STATE);

    Os_Arch_PrepareState(task);
    Os_PriorityType prio = Os_TaskPrio(task);
    Os_ReadyListPushTail(&Os_TaskReady[prio], task);
    Os_TaskControls[task].state = OS_TASK_READY;
}

/**
 * @brief Perform the state transition from ready to running
 * @param task task to transition to the running state
 *
 * This will pop the task out from the ready list, set it
 * as the running task.
 */
static __inline void Os_State_Ready_To_Running(Os_TaskType task)
{
    OS_CHECK_EXT(Os_TaskControls[task].state == OS_TASK_READY   , E_OS_STATE);

    Os_PriorityType prio = Os_TaskPrio(task);
    Os_TaskType     task2;
    Os_ReadyListPopHead(&Os_TaskReady[prio], &task2);

    OS_CHECK_EXT(task2 == task, E_OS_STATE);

    Os_TaskControls[task].state = OS_TASK_RUNNING;
    Os_TaskRunning = task;

    OS_PRETASKHOOK(task);
}

/**
 * @brief Start scheduler activity
 */
void Os_Start(void)
{

    Os_TaskType     task;
    Os_TaskPeek(0, &task);
    if (task == OS_INVALID_TASK) {
        /* no task found, just wait for tick to trigger one */
        Os_Arch_EnableAllInterrupts();
        while(Os_Continue) {
            ; /* NOP */
        }
    } else {
        /* pop this task out from ready */
        Os_State_Ready_To_Running(task);

        /* call context will be task after this */
        Os_CallContext = OS_CONTEXT_TASK;

        /* re-grab internal resource if not held */
        Os_TaskInternalResource_Get();

        /* swap into first task */
        Os_Arch_SwapState(task, OS_INVALID_TASK);

        while(Os_Continue) {
            ; /* NOP */
        }
    }
}

void Os_Shutdown(void)
{
    Os_Arch_DisableAllInterrupts();

    Os_Continue = FALSE;
    /* store previous to be able to swap state later */
    Os_TaskType prev = Os_TaskRunning;

    if (prev != OS_INVALID_TASK) {
        /* put preempted task as first ready */
        Os_State_Running_To_Ready(prev);
    }

    /* swap back if os support it */
    Os_Arch_SwapState(OS_INVALID_TASK, prev);
}

/**
 * @brief Main scheduling function
 * @return E_OK on success
 *
 * This function performs task switching to highest priority task. If
 * called from task context, it will not always return directly but
 * another higher priority task may execute before control is returned
 * to caller.
 *
 * Call contexts: TASK, (ISR1 from Os)
 */
Os_StatusType Os_Schedule_Internal(void)
{
    Os_PriorityType prio;
    Os_TaskType     task, prev;

    if (Os_TaskRunning == OS_INVALID_TASK) {
        prio = 0u;
    } else {
        prio = Os_TaskPrio(Os_TaskRunning) + 1;
    }

    while(1) {
        Os_TaskPeek(prio, &task);

        if (task == OS_INVALID_TASK) {
            if (Os_TaskRunning != OS_INVALID_TASK) {
                break;    /* continue with current */
            }
            if (Os_CallContext != OS_CONTEXT_TASK) {
                break;    /* no new task, and we are inside tick, so we need to return out */
            }

            /* idling in last task */
            continue;
        }

        /* store previous to be able to swap state later */
        prev = Os_TaskRunning;

        if (prev != OS_INVALID_TASK) {
            /* put preempted task as first ready */
            Os_State_Running_To_Ready(prev);
        }

        /* pop this task out from ready */
        Os_State_Ready_To_Running(task);

        /* re-grab internal resource if not held */
        Os_TaskInternalResource_Get();

        /* no direct return if called from task
         * if called from ISR, this will just
         * prepare next task. function can use
         * current running task to check if it
         * just restored to this state */
        Os_Arch_SwapState(task, prev);
        break;
    }
    return E_OK;
}

/** @copydoc Os_Schedule_Internal */
Os_StatusType Os_Schedule(void)
{
    Os_StatusType result;
    Os_Arch_DisableAllInterrupts();

    Os_TaskInternalResource_Release();

    result = Os_Schedule_Internal();

    Os_TaskInternalResource_Get();

    Os_Arch_EnableAllInterrupts();
    return result;
}

void Os_Isr(void)
{
    Os_CallContext = OS_CONTEXT_ISR1;
    Os_Ticks++;
    Os_AlarmTick();
    Os_Schedule_Internal();
    Os_CallContext = OS_CONTEXT_TASK;
}

/**
 * @brief Terminate calling task
 * @return
 *  - E_OK on success
 *  - E_OS_LIMIT if activation is already zero
 *  - E_OS_RESOURCE if still holding a resource
 *  - E_OS_* see Os_Schedule_Internal()
 *
 * The function will terminate the calling task and transfer execution to
 * next in line to execute. If task already have a queued activation it
 * will be put into end end of the ready list again.
 *
 * Call contexts: TASK
 */
Os_StatusType Os_TerminateTask_Internal(void)
{
    OS_CHECK_EXT_R(Os_TaskControls[Os_TaskRunning].state == OS_TASK_RUNNING       , E_OS_STATE);
    OS_CHECK_EXT_R(Os_CallContext == OS_CONTEXT_TASK                              , E_OS_CALLEVEL);
    OS_CHECK_EXT_R(Os_TaskControls[Os_TaskRunning].resource == OS_INVALID_RESOURCE, E_OS_RESOURCE);

    Os_State_Running_To_Suspended(Os_TaskRunning);

#if( (OS_CONFORMANCE == OS_CONFORMANCE_ECC2) ||  (OS_CONFORMANCE == OS_CONFORMANCE_BCC2) )
    Os_TaskControls[Os_TaskRunning].activation--;
    if (Os_TaskControls[Os_TaskRunning].activation) {
        Os_State_Suspended_To_Ready(Os_TaskRunning);
    }
#endif

    Os_TaskRunning = OS_INVALID_TASK;
    return E_OK;

OS_ERRORCHECK_EXIT_POINT:
    Os_Error.service = OSServiceId_TerminateTask;
    OS_ERRORHOOK(Os_Error.status);
    return Os_Error.status;
}

/** @copydoc Os_TerminateTask_Internal */
Os_StatusType Os_TerminateTask(void)
{
    Os_StatusType result;
    Os_Arch_DisableAllInterrupts();

    Os_TaskInternalResource_Release();

    result = Os_TerminateTask_Internal();
    if (result == E_OK) {
        result = Os_Schedule_Internal();
    }

    Os_TaskInternalResource_Get();

    Os_Arch_EnableAllInterrupts();
    return result;
}


/**
 * @brief Terminate calling task and chain into given task
 * @return
 *  - E_OK on success
 *  - E_OS_LIMIT if activation is already zero
 *  - E_OS_RESOURCE if still holding a resource
 *  - E_OS_* see Os_Schedule_Internal()
 *
 * This service causes the termination of the calling task. After
 * termination of the calling task a succeeding task <TaskID> is
 * activated. Using this service, it ensures that the succeeding task
 * starts to run at the earliest after the calling task has been
 * terminated.
 *
 * If the succeeding task is identical with the current task, this
 * does not result in multiple requests. The task is not transferred
 * to the suspended state, but will immediately become ready
 * again.
 *
 * An internal resource assigned to the calling task is automatically
 * released, even if the succeeding task is identical with the current
 * task. Other resources occupied by the calling shall have
 * been released before ChainTask is called. If a resource is still
 * occupied in standard status the behaviour is undefined.
 * If called successfully, ChainTask does not return to the call level
 * and the status can not be evaluated.
 * In case of error the service returns to the calling task and
 * provides a status which can then be evaluated in the
 * application.
 * If the service ChainTask is called successfully, this enforces a
 * rescheduling.
 * Ending a task function without call to TerminateTask or
 * ChainTask is strictly forbidden and may leave the system in an
 * undefined state.
 * If E_OS_LIMIT is returned the activation is ignored.
 * When an extended task is transferred from suspended state
 * into ready state all its events are cleared.
 *
 * Call contexts: TASK
 */
Os_StatusType Os_ChainTask_Internal(Os_TaskType task)
{
    OS_CHECK_EXT_R(Os_TaskControls[Os_TaskRunning].state == OS_TASK_RUNNING       , E_OS_STATE);
    OS_CHECK_EXT_R(Os_CallContext == OS_CONTEXT_TASK                              , E_OS_CALLEVEL);
    OS_CHECK_EXT_R(Os_TaskControls[Os_TaskRunning].resource == OS_INVALID_RESOURCE, E_OS_RESOURCE);
    OS_CHECK_EXT_R(task < OS_TASK_COUNT                                           , E_OS_ID);

    Os_State_Running_To_Suspended(Os_TaskRunning);

#if( (OS_CONFORMANCE == OS_CONFORMANCE_ECC2) ||  (OS_CONFORMANCE == OS_CONFORMANCE_BCC2) )
    Os_TaskControls[Os_TaskRunning].activation--;
    if (Os_TaskControls[Os_TaskRunning].activation > 0u) {
        Os_State_Suspended_To_Ready(Os_TaskRunning);
    }
#endif

    Os_TaskRunning = OS_INVALID_TASK;

    /* may return early here after activation limit is reached */
#if( (OS_CONFORMANCE == OS_CONFORMANCE_ECC2) ||  (OS_CONFORMANCE == OS_CONFORMANCE_BCC2) )
    OS_CHECK_R    (Os_TaskControls[task].activation < Os_TaskConfigs[task].activation, E_OS_LIMIT);

    Os_TaskControls[task].activation++;
    if (Os_TaskControls[task].activation == 1u) {
        Os_State_Suspended_To_Ready(task);
    }
#else
    OS_CHECK_R    (Os_TaskControls[task].state == OS_TASK_SUSPENDED, E_OS_LIMIT);
    Os_State_Suspended_To_Ready(task);
#endif

    return E_OK;

OS_ERRORCHECK_EXIT_POINT:
    Os_Error.service   = OSServiceId_ChainTask;
    Os_Error.params[0] = task;
    OS_ERRORHOOK(Os_Error.status);
    return Os_Error.status;
}

/** @copydoc Os_ChainTask_Internal */
Os_StatusType Os_ChainTask(Os_TaskType task)
{
    Os_StatusType result;
    Os_Arch_DisableAllInterrupts();

    Os_TaskInternalResource_Release();

    result = Os_ChainTask_Internal(task);
    if (result == E_OK) {
        result = Os_Schedule_Internal();
    }

    Os_TaskInternalResource_Get();

    Os_Arch_EnableAllInterrupts();
    return result;
}

/**
 * @brief Increase activation count for task by one
 * @param task Task to activate
 * @return
 *  - E_OK on success
 *  - E_OS_LIMIT if maximum activations have been reached
 *  - E_OS_* (see Os_Schedule_Internal)
 *
 * This will transfer a task from a suspended state to a ready state.
 *
 * Call contexts: TASK, ISR2
 */
static Os_StatusType Os_ActivateTask_Internal(Os_TaskType task)
{
    OS_CHECK_EXT_R(task < OS_TASK_COUNT                   , E_OS_ID);

#if( (OS_CONFORMANCE == OS_CONFORMANCE_ECC2) ||  (OS_CONFORMANCE == OS_CONFORMANCE_BCC2) )
    OS_CHECK_R    (Os_TaskControls[task].activation < Os_TaskConfigs[task].activation, E_OS_LIMIT);

    Os_TaskControls[task].activation++;
    if (Os_TaskControls[task].activation == 1u) {
        Os_State_Suspended_To_Ready(task);
    }
#else
    OS_CHECK_EXT_R(Os_TaskControls[task].state == OS_TASK_SUSPENDED, E_OS_LIMIT);
    Os_State_Suspended_To_Ready(task);
#endif
    return E_OK;

OS_ERRORCHECK_EXIT_POINT:
    Os_Error.service   = OSServiceId_ActivateTask;
    Os_Error.params[0] = task;
    OS_ERRORHOOK(Os_Error.status);
    return Os_Error.status;
}

/** @copydoc Os_ActivateTask_Internal */
Os_StatusType Os_ActivateTask(Os_TaskType task)
{
    Os_StatusType result;
    Os_Arch_DisableAllInterrupts();
    result = Os_ActivateTask_Internal(task);
    if (result == E_OK) {
        result = Os_Schedule_Internal();
    }
    Os_Arch_EnableAllInterrupts();
    return result;
}

/**
 * @brief Lock resource for active task
 * @param res
 * @return
 *  - E_OK on success
 *  - E_OS_ID on invalid resource
 *  - E_OS_ACCESS Attempt to get a resource which is already occupied by any task
 *                or ISR, or the statically assigned priority of the calling task or
 *                interrupt routine is higher than the calculated ceiling priority,
 *
 * Call contexts: TASK, ISR2
 */
static Os_StatusType Os_GetResource_Task(Os_ResourceType res)
{
    OS_CHECK_EXT_R(res < OS_RES_COUNT                                               , E_OS_ID);
    OS_CHECK_EXT_R(Os_ResourceControls[res].task == OS_INVALID_TASK                 , E_OS_ACCESS);
    OS_CHECK_EXT_R(Os_TaskPrio(Os_TaskRunning)   <= Os_ResourceConfigs[res].priority, E_OS_ACCESS);
    Os_ResourceControls[res].task            = Os_TaskRunning;
    Os_ResourceControls[res].next            = Os_TaskControls[Os_TaskRunning].resource;
    Os_TaskControls[Os_TaskRunning].resource = res;

    return E_OK;

OS_ERRORCHECK_EXIT_POINT:
    Os_Error.service   = OSServiceId_GetResource;
    Os_Error.params[0] = res;
    OS_ERRORHOOK(Os_Error.status);
    return Os_Error.status;
}

static Os_StatusType Os_GetResource_Isr(Os_ResourceType res)
{
    OS_CHECK_EXT_R(0, E_OS_SYS_NOT_IMPLEMENTED);

    return E_OK;

OS_ERRORCHECK_EXIT_POINT:
    Os_Error.service   = OSServiceId_GetResource;
    Os_Error.params[0] = res;
    OS_ERRORHOOK(Os_Error.status);
    return Os_Error.status;
}

/** @copydoc Os_GetResource_Task */
Os_StatusType Os_GetResource(Os_ResourceType res)
{
    Os_StatusType result;
    Os_Arch_DisableAllInterrupts();
    if (Os_CallContext == OS_CONTEXT_TASK) {
        result = Os_GetResource_Task(res);
    } else {
        result = Os_GetResource_Isr(res);
        OS_CHECK_EXT_R(result == E_OK, result);
    }
    Os_Arch_EnableAllInterrupts();
    return result;

OS_ERRORCHECK_EXIT_POINT:
    Os_Error.service   = OSServiceId_GetResource;
    Os_Error.params[0] = res;
    OS_ERRORHOOK(Os_Error.status);
    return Os_Error.status;
}

/**
 * @brief Release resource for active task
 * @param res
 * @return
 *  - E_OK on success
 *  - E_OS_ID on invalid resource
 *  - E_OS_NOFUNC Attempt to release a resource which is not occupied by any task
 *                or ISR, or another resource shall be released before.
 *  - E_OS_ACCESS Attempt to release a resource which has a lower ceiling priority
 *                than the statically assigned priority of the calling task or
 *                interrupt routine.
 *
 * Call contexts: TASK, ISR2
 */
Os_StatusType Os_ReleaseResource_Task(Os_ResourceType res)
{
    OS_CHECK_EXT_R(res < OS_RES_COUNT                             , E_OS_ID);
    OS_CHECK_EXT_R(Os_ResourceControls[res].task == Os_TaskRunning, E_OS_NOFUNC);
    OS_CHECK_EXT_R(Os_TaskControls[Os_TaskRunning].resource == res, E_OS_NOFUNC);

    Os_TaskControls[Os_TaskRunning].resource = Os_ResourceControls[res].next;
    Os_ResourceControls[res].task  = OS_INVALID_TASK;
    Os_ResourceControls[res].next  = OS_INVALID_RESOURCE;

    return E_OK;

OS_ERRORCHECK_EXIT_POINT:
    Os_Error.service   = OSServiceId_ReleaseResource;
    Os_Error.params[0] = res;
    OS_ERRORHOOK(Os_Error.status);
    return Os_Error.status;
}

/** @copydoc Os_ReleaseResource_Task */
Os_StatusType Os_ReleaseResource(Os_ResourceType res)
{
    Os_StatusType result;
    Os_Arch_DisableAllInterrupts();
    if (Os_CallContext == OS_CONTEXT_TASK) {
        result = Os_ReleaseResource_Task(res);
    } else {
        result = E_OS_SYS_NOT_IMPLEMENTED;
    }
    if (result == E_OK) {
        result = Os_Schedule_Internal();
    }
    Os_Arch_EnableAllInterrupts();
    return result;
}

#ifdef OS_ALARM_COUNT

/**
 * @brief Initialize the alarm context
 * @param alarm alarm to initialize
 */
void Os_AlarmInit(Os_AlarmType alarm)
{
    Os_AlarmControls[alarm].cycle = 0u;
    Os_AlarmControls[alarm].next  = OS_INVALID_ALARM;
    Os_AlarmControls[alarm].ticks = 0u;
}

/**
 * @brief Set the alarm to trigger after a relative time
 * @param[in] alarm     Reference to the alarm element
 * @param[in] increment Relative value in ticks
 * @param[in] cycle     Cycle value in case of cyclic alarm. In case of single alarms, cycle shall be zero
 * @return
 *  - E_OK        No error
 *  - E_OS_STATE  Alarm <AlarmID> is already in use
 *  - E_OS_ID     Alarm <AlarmID> is invalid
 *  - E_OS_VALUE  Value of <increment> outside of the admissible
 *                limits (lower than zero or greater than maxallowedvalue)
 *  - E_OS_VALUE  Value of <cycle> unequal to 0 and outside of the admissible
 *                counter limits (less than mincycle or greater than maxallowedvalue)
 *
 * The system service occupies the alarm <AlarmID> element.
 * After <increment> ticks have elapsed, the task assigned to the
 * alarm <AlarmID> is activated or the assigned event (only for
 * extended tasks) is set or the alarm-callback routine is called.
 *
 * If the relative value <increment> is very small, the alarm may
 * expire, and the task may become ready or the alarm-callback
 * may be called before the system service returns to the user.
 * If <cycle> is unequal zero, the alarm element is logged on again
 * immediately after expiry with the relative value <cycle>.
 * The alarm <AlarmID> must not already be in use.
 * To change values of alarms already in use the alarm shall be
 * cancelled first.
 * If the alarm is already in use, this call will be ignored and the
 * error E_OS_STATE is returned.
 *
 * Call contexts: TASK, ISR2
 */
Os_StatusType Os_SetRelAlarm_Internal(Os_AlarmType alarm, Os_TickType increment, Os_TickType cycle)
{
    OS_CHECK_EXT_R(alarm < OS_ALARM_COUNT             , E_OS_ID);
    OS_CHECK_R    (Os_AlarmControls[alarm].ticks == 0u, E_OS_ID); /* TODO can fail this check */
    Os_AlarmControls[alarm].cycle = cycle;
    Os_AlarmAdd(alarm, increment);
    return E_OK;

OS_ERRORCHECK_EXIT_POINT:
    Os_Error.service   = OSServiceId_SetRelAlarm;
    Os_Error.params[0] = alarm;
    Os_Error.params[1] = increment;
    Os_Error.params[2] = cycle;
    OS_ERRORHOOK(Os_Error.status);
    return Os_Error.status;
}

/** @copydoc Os_SetRelAlarm_Internal */
Os_StatusType Os_SetRelAlarm(Os_AlarmType alarm, Os_TickType increment, Os_TickType cycle)
{
    Os_StatusType result;
    Os_Arch_DisableAllInterrupts();
    result = Os_SetRelAlarm_Internal(alarm, increment, cycle);
    Os_Arch_EnableAllInterrupts();
    return result;
}

/**
 * @brief Starts an absolute alarm
 * @param[in] alarm Reference to the alarm element
 * @param[in] start Absolute value in ticks
 * @param[in] cycle Cycle value in case of cyclic alarm. In case of single alarms, cycle shall be zero
 * @return
 *  - E_OK        No error
 *  - E_OS_STATE  Alarm <AlarmID> is already in use
 *  - E_OS_ID     Alarm <AlarmID> is invalid
 *  - E_OS_VALUE  Value of <start> outside of the admissible
 *                counter limit (less than zero or greater than maxallowedvalue)
 *  - E_OS_VALUE  Value of <cycle> unequal to 0 and outside of the admissible
 *                counter limits (less than mincycle or greater than maxallowedvalue)
 *
 * The system service occupies the alarm <AlarmID> element.
 * When <start> ticks are reached, the task assigned to the alarm
 * <AlarmID> is activated or the assigned event (only for extended
 * tasks) is set or the alarm-callback routine is called.
 *
 * If the absolute value <start> is very close to the current counter
 * value, the alarm may expire, and the task may become ready or
 * the alarm-callback may be called before the system service
 * returns to the user.
 * If the absolute value <start> already was reached before the
 * system call, the alarm shall only expire when the absolute value
 * <start> is reached again, i.e. after the next overrun of the
 * counter.
 * If <cycle> is unequal zero, the alarm element is logged on again
 * immediately after expiry with the relative value <cycle>.
 * The alarm <AlarmID> shall not already be in use.
 * To change values of alarms already in use the alarm shall be
 * cancelled first.
 * If the alarm is already in use, this call will be ignored and the
 * error E_OS_STATE is returned.
 *
 * Call contexts: TASK, ISR2
 */
Os_StatusType Os_SetAbsAlarm_Internal(Os_AlarmType alarm, Os_TickType start, Os_TickType cycle)
{
    OS_CHECK_EXT_R(alarm < OS_ALARM_COUNT, E_OS_ID);
    OS_CHECK_R    (Os_AlarmControls[alarm].ticks == 0u, E_OS_ID); /* TODO can fail this check */
    Os_AlarmControls[alarm].cycle = cycle;
    if (start >= Os_Ticks) {
        Os_AlarmAdd(alarm, start - Os_Ticks);
    } else {
        Os_AlarmAdd(alarm, OS_MAXALLOWEDVALUE - Os_Ticks + start + 1u);
    }
    return E_OK;

OS_ERRORCHECK_EXIT_POINT:
    Os_Error.service   = OSServiceId_SetAbsAlarm;
    Os_Error.params[0] = alarm;
    Os_Error.params[1] = start;
    Os_Error.params[2] = cycle;
    OS_ERRORHOOK(Os_Error.status);
    return Os_Error.status;
}

/** @copydoc Os_SetAbsAlarm */
Os_StatusType Os_SetAbsAlarm(Os_AlarmType alarm, Os_TickType start, Os_TickType cycle)
{
    Os_StatusType result;
    Os_Arch_DisableAllInterrupts();
    result = Os_SetAbsAlarm_Internal(alarm, start, cycle);
    Os_Arch_EnableAllInterrupts();
    return result;
}

/**
 * @brief Cancels a running alarm
 * @param alarm Reference to an alarm
 * @return
 *  - E_OS_NOFUNC Alarm <AlarmID> not in use
 *  - E_OS_ID     Alarm <AlarmID> is invalid
 *
 * Call contexts: TASK, ISR2
 */
Os_StatusType Os_CancelAlarm_Internal(Os_AlarmType alarm)
{
    Os_AlarmType next = Os_AlarmNext
               , prev = OS_INVALID_ALARM;

    OS_CHECK_EXT_R(alarm < OS_ALARM_COUNT   , E_OS_ID);

    while (next != OS_INVALID_ALARM && next != alarm) {
        prev = next;
        next = Os_AlarmControls[next].next;
    }

    OS_CHECK_R(next == alarm, E_OS_NOFUNC);

    next = Os_AlarmControls[alarm].next;
    if (prev == OS_INVALID_ALARM) {
        Os_AlarmNext = next;
    } else {
        Os_AlarmControls[prev].next = next ;
    }

    if (next != OS_INVALID_ALARM) {
        Os_AlarmControls[next].ticks += Os_AlarmControls[alarm].ticks;
    }

    Os_AlarmControls[alarm].ticks = 0u;
    Os_AlarmControls[alarm].cycle = 0u;
    Os_AlarmControls[alarm].next  = OS_INVALID_ALARM;
    return E_OK;

OS_ERRORCHECK_EXIT_POINT:
    Os_Error.service   = OSServiceId_CancelAlarm;
    Os_Error.params[0] = alarm;
    OS_ERRORHOOK(Os_Error.status);
    return Os_Error.status;
}

/** @copydoc Os_CancelAlarm_Internal */
Os_StatusType Os_CancelAlarm(Os_AlarmType alarm)
{
    Os_StatusType result;
    Os_Arch_DisableAllInterrupts();
    result = Os_CancelAlarm_Internal(alarm);
    Os_Arch_EnableAllInterrupts();
    return result;
}

/**
 * @brief The system service GetAlarm returns the relative value in ticks before the alarm <AlarmID> expires.
 * @param[in]  alarm Reference to an alarm
 * @param[out] tick  Relative value in ticks before the alarm <AlarmID> expires
 * @return
 *  - E_OK no error
 *  - E_OS_NOFUNC Alarm <AlarmID> is not used
 *  - E_OS_ID Alarm <AlarmID> is invalid
 *
 * It is up to the application to decide whether for example a
 * CancelAlarm may still be useful.
 * If <AlarmID> is not in use, <Tick> is not defined.
 *
 * Call contexts: TASK, ISR2, HOOKS
 */
Os_StatusType Os_GetAlarm_Internal(Os_AlarmType alarm, Os_TickType* tick)
{
    Os_AlarmType index;

    OS_CHECK_EXT_R(alarm < OS_ALARM_COUNT, E_OS_ID);

    *tick = 0u;
    index = Os_AlarmNext;
    while (index != OS_INVALID_ALARM && index != alarm) {
        index  = Os_AlarmControls[index].next;
        *tick += Os_AlarmControls[index].ticks;
    }

    OS_CHECK_R(index != OS_INVALID_ALARM, E_OS_NOFUNC);

    *tick += Os_AlarmControls[index].ticks;
    return E_OK;

OS_ERRORCHECK_EXIT_POINT:
    Os_Error.service   = OSServiceId_GetAlarm;
    Os_Error.params[0] = alarm;
    OS_ERRORHOOK(Os_Error.status);
    return Os_Error.status;
}

/** @copydoc Os_CancelAlarm_Internal */
Os_StatusType Os_GetAlarm(Os_AlarmType alarm, Os_TickType* tick)
{
    Os_StatusType result;
    Os_Arch_DisableAllInterrupts();
    result = Os_GetAlarm_Internal(alarm, tick);
    Os_Arch_EnableAllInterrupts();
    return result;
}

#endif /* OS_ALARM_COUNT */

/**
 * @brief Initializes OS internal structures with given config
 * @param config Configuration to use
 */
void Os_Init(const Os_ConfigType* config)
{
    Os_TaskType     task;
    Os_ResourceType res;
    Os_AlarmType    alarm;
    uint_least8_t prio;

    Os_TaskConfigs     = *config->tasks;
    Os_ResourceConfigs = *config->resources;
    Os_AlarmConfigs    = *config->alarms;
    Os_CallContext     = OS_CONTEXT_NONE;
    Os_TaskRunning     = OS_INVALID_TASK;
    Os_Ticks           = 0u;
    Os_AlarmNext       = OS_INVALID_TASK;
    Os_Continue        = TRUE;

    memset(&Os_TaskControls    , 0u, sizeof(Os_TaskControls));
    memset(&Os_ResourceControls, 0u, sizeof(Os_ResourceControls));

    for (prio = 0u; prio < OS_PRIO_COUNT; ++prio) {
        Os_ReadyListInit(&Os_TaskReady[prio]);
    }

    /* initialize task */
    for (task = 0u; task < OS_TASK_COUNT; ++task) {
        Os_TaskInit(task);
    }

    /* initialize resources */
    for (res  = 0u; res  < OS_RES_COUNT; ++res) {
        Os_ResourceInit(res);
    }

    /* initialize alarms */
    for (alarm  = 0u; alarm  < OS_ALARM_COUNT; ++alarm) {
        Os_AlarmInit(alarm);
    }

    /* run arch init */
    Os_Arch_Init();

    /* make sure any activated task is in ready list */
    for (task = 0u; task < OS_TASK_COUNT; ++task) {

        /* check some task config */
        Os_ResourceType res = Os_TaskConfigs[task].resource;
        if (res != OS_INVALID_TASK) {
            OS_CHECK_EXT(Os_TaskConfigs[task].priority <= Os_ResourceConfigs[res].priority, E_OS_RESOURCE);
        }

        /* ready any activated task */
        if (Os_TaskConfigs[task].autostart) {
#if( (OS_CONFORMANCE == OS_CONFORMANCE_ECC2) ||  (OS_CONFORMANCE == OS_CONFORMANCE_BCC2) )
            Os_TaskControls[task].activation = 1;
#endif
            Os_State_Suspended_To_Ready(task);
        }
    }
}


