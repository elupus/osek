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

Os_TaskControlType              Os_TaskControls        [OS_TASK_COUNT]; /**< control array for tasks */
Os_ReadyListType                Os_TaskReady           [OS_PRIO_COUNT]; /**< array of ready lists based on priority */
Os_TaskType                     Os_TaskRunning;                         /**< currently running task */
Os_ContextType                  Os_CallContext;                         /**< current call context */
const Os_TaskConfigType *       Os_TaskConfigs;                         /**< config array for tasks */

const Os_ResourceConfigType *   Os_ResourceConfigs;                     /**< config array for resources */
Os_ResourceControlType          Os_ResourceControls    [OS_RES_COUNT];  /**< control array for resources */

static StatusType Os_Schedule_Internal(void);
static StatusType Os_TerminateTask_Internal(void);
static StatusType Os_ActivateTask_Internal(Os_TaskType task);
static StatusType Os_GetResource_Internal(Os_ResourceType res);
static StatusType Os_ReleaseResource_Internal(Os_ResourceType res);

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
 * @brief Initializes task controls based on config
 * @param[in] task the task to initialize
 */
static void Os_TaskInit(Os_TaskType task)
{
    memset(&Os_TaskControls[task], 0, sizeof(Os_TaskControls[task]));
    Os_TaskControls[task].next     = OS_INVALID_TASK;
    Os_TaskControls[task].resource = OS_INVALID_RESOURCE;
    if (Os_TaskConfigs[task].autostart) {
        Os_TaskControls[task].activation = 1;
    }
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

    OS_ERRORCHECK(Os_CallContext == OS_CONTEXT_TASK, E_OS_STATE);
    OS_ERRORCHECK(Os_TaskRunning != OS_INVALID_TASK  , E_OS_STATE);

    res = Os_TaskConfigs[Os_TaskRunning].resource;
    if (res != OS_INVALID_RESOURCE) {
        (void)Os_ReleaseResource_Internal(res);
    }
}

/**
 * @brief Get the internal resource of running task
 *
 * If the running (or to be running) task has an associated internal
 * resource, this function will make sure the task holds that resource.
 *
 * Can only be called from task context.
 */
void Os_TaskInternalResource_Get(void)
{
    Os_ResourceType res;

    OS_ERRORCHECK(Os_CallContext == OS_CONTEXT_TASK, E_OS_STATE);
    OS_ERRORCHECK(Os_TaskRunning != OS_INVALID_TASK  , E_OS_STATE);

    res = Os_TaskConfigs[Os_TaskRunning].resource;
    if (res != OS_INVALID_RESOURCE) {
        if (Os_ResourceControls[res].task != Os_TaskRunning) {
            (void)Os_GetResource_Internal(res);
        }
    }
}

/**
 * @brief Perform the state transition from running to suspended for a task
 * @param task task to transition to suspended state
 */
static __inline void Os_State_Running_To_Suspended(Os_TaskType task)
{
    OS_ERRORCHECK(Os_TaskControls[task].state == OS_TASK_RUNNING, E_OS_STATE);

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
    OS_ERRORCHECK(Os_TaskControls[task].state == OS_TASK_RUNNING, E_OS_STATE);

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
    OS_ERRORCHECK(Os_TaskControls[task].state == OS_TASK_SUSPENDED, E_OS_STATE);

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
    OS_ERRORCHECK(Os_TaskControls[task].state == OS_TASK_READY   , E_OS_STATE);

    Os_PriorityType prio = Os_TaskPrio(task);
    Os_TaskType     task2;
    Os_ReadyListPopHead(&Os_TaskReady[prio], &task2);

    OS_ERRORCHECK(task2 == task, E_OS_STATE);

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
        while(1) {
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

        /* should never be reached */
        OS_ERRORCHECK(0, E_OS_NOFUNC);
        while(1) {
            ; /* NOP */
        }
    }
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
StatusType Os_Schedule_Internal(void)
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
StatusType Os_Schedule(void)
{
    StatusType result;
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
StatusType Os_TerminateTask_Internal(void)
{
    OS_ERRORCHECK_R(Os_TaskControls[Os_TaskRunning].activation > 0               , E_OS_LIMIT);
    OS_ERRORCHECK_R(Os_CallContext == OS_CONTEXT_TASK                            , E_OS_CALLEVEL);
    OS_ERRORCHECK_R(Os_TaskControls[Os_TaskRunning].resource == OS_INVALID_RESOURCE, E_OS_RESOURCE);

    Os_State_Running_To_Suspended(Os_TaskRunning);

    Os_TaskControls[Os_TaskRunning].activation--;
    if (Os_TaskControls[Os_TaskRunning].activation) {
        Os_State_Suspended_To_Ready(Os_TaskRunning);
    }

    Os_TaskRunning = OS_INVALID_TASK;
    return Os_Schedule_Internal();
}

/** @copydoc Os_TerminateTask_Internal */
StatusType Os_TerminateTask(void)
{
    StatusType result;
    Os_Arch_DisableAllInterrupts();

    Os_TaskInternalResource_Release();

    result = Os_TerminateTask_Internal();

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
static StatusType Os_ActivateTask_Internal(Os_TaskType task)
{
    OS_ERRORCHECK_R(task < OS_TASK_COUNT                   , E_OS_ID);
    OS_ERRORCHECK_R(Os_TaskControls[task].activation <  255, E_OS_LIMIT);

    Os_TaskControls[task].activation++;
    if (Os_TaskControls[task].activation == 1u) {
        Os_State_Suspended_To_Ready(task);
    }

    return Os_Schedule_Internal();
}

/** @copydoc Os_ActivateTask_Internal */
StatusType Os_ActivateTask(Os_TaskType task)
{
    StatusType result;
    Os_Arch_DisableAllInterrupts();
    result = Os_ActivateTask_Internal(task);
    Os_Arch_EnableAllInterrupts();
    return result;
}

/**
 * @brief Lock resource for active task/ISR2
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
static StatusType Os_GetResource_Internal(Os_ResourceType res)
{
    OS_ERRORCHECK_R(res < OS_RES_COUNT, E_OS_ID);
    OS_ERRORCHECK_R(Os_ResourceControls[res].task == OS_INVALID_TASK         , E_OS_ACCESS);

    if (Os_CallContext == OS_CONTEXT_TASK) {
        OS_ERRORCHECK_R(Os_TaskPrio(Os_TaskRunning)   <= Os_ResourceConfigs[res].priority, E_OS_ACCESS);
        Os_ResourceControls[res].task            = Os_TaskRunning;
        Os_ResourceControls[res].next            = Os_TaskControls[Os_TaskRunning].resource;
        Os_TaskControls[Os_TaskRunning].resource = res;
    } else {
        OS_ERRORCHECK_R(0, E_OS_SYS_NOT_IMPLEMENTED);
    }

    return E_OK;
}


/** @copydoc Os_GetResource_Internal */
StatusType Os_GetResource(Os_ResourceType res)
{
    StatusType result;
    Os_Arch_DisableAllInterrupts();
    result = Os_GetResource_Internal(res);
    Os_Arch_EnableAllInterrupts();
    return result;
}


/**
 * @brief Release resource for active task/ISR2
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
StatusType Os_ReleaseResource_Internal(Os_ResourceType res)
{
    OS_ERRORCHECK_R(res < OS_RES_COUNT, E_OS_ID);

    if (Os_CallContext == OS_CONTEXT_TASK) {
        OS_ERRORCHECK_R(Os_ResourceControls[res].task == Os_TaskRunning, E_OS_NOFUNC);
        OS_ERRORCHECK_R(Os_TaskControls[Os_TaskRunning].resource == res, E_OS_NOFUNC);

        Os_TaskControls[Os_TaskRunning].resource = Os_ResourceControls[res].next;
        Os_ResourceControls[res].task  = OS_INVALID_TASK;
        Os_ResourceControls[res].next  = OS_INVALID_RESOURCE;
    } else {
        OS_ERRORCHECK_R(0, E_OS_SYS_NOT_IMPLEMENTED);
    }

    return Os_Schedule_Internal();
}


/** @copydoc Os_ReleaseResource_Internal */
StatusType Os_ReleaseResource(Os_ResourceType res)
{
    StatusType result;
    Os_Arch_DisableAllInterrupts();
    result = Os_ReleaseResource_Internal(res);
    Os_Arch_EnableAllInterrupts();
    return result;
}

/**
 * @brief Initializes OS internal structures with given config
 * @param config Configuration to use
 */
void Os_Init(const Os_ConfigType* config)
{
    Os_TaskType     task;
    Os_ResourceType res;
    uint_least8_t prio;

    Os_TaskConfigs     = *config->tasks;
    Os_ResourceConfigs = *config->resources;
    Os_CallContext     = OS_CONTEXT_NONE;
    Os_TaskRunning     = OS_INVALID_TASK;

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

    /* run arch init */
    Os_Arch_Init();

    /* make sure any activated task is in ready list */
    for (task = 0u; task < OS_TASK_COUNT; ++task) {

        /* check some task config */
        Os_ResourceType res = Os_TaskConfigs[task].resource;
        if (res != OS_INVALID_TASK) {
            OS_ERRORCHECK(Os_TaskConfigs[task].priority <= Os_ResourceConfigs[res].priority, E_OS_RESOURCE);
        }

        /* ready any activated task */
        if (Os_TaskControls[task].activation) {
            Os_State_Suspended_To_Ready(task);
        }
    }
}


