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
#include <stdarg.h>

#include "Std_Types.h"
#include "Os_Types.h"
#include "Os.h"

Os_ErrorType                    Os_Error;
Os_TaskControlType              Os_TaskControls        [OS_TASK_COUNT]; /**< control array for tasks */
Os_ReadyListType                Os_TaskReady           [OS_PRIO_COUNT]; /**< array of ready lists based on priority */
Os_TaskType                     Os_ActiveTask;                         /**< currently running task */
Os_ContextType                  Os_CallContext;                         /**< current call context */
const Os_TaskConfigType *       Os_TaskConfigs;                         /**< config array for tasks */

const Os_ResourceConfigType *   Os_ResourceConfigs;                     /**< config array for resources */
Os_ResourceControlType          Os_ResourceControls    [OS_RES_COUNT];  /**< control array for resources */

volatile boolean                Os_Continue;                            /**< should starting task continue */
boolean                         Os_Initialized;                         /**< Os_Init accepted the configuration */
Os_AppModeType                  Os_AppMode;                             /**< application mode the OS was started in */
#if(OS_ERRORHOOK_ENABLE)
boolean                         Os_ErrorHookActive;                     /**< error hook is running */
#endif


#ifdef OS_ALARM_COUNT
Os_TickType                     Os_AlarmTicks          [OS_ALARM_COUNT]; /**< ticks for alarms */
Os_TickType                     Os_AlarmCycles         [OS_ALARM_COUNT]; /**< @brief number of ticks in each cycle */
boolean                         Os_AlarmQueued         [OS_ALARM_COUNT]; /**< @brief is this alarm active */

const Os_AlarmConfigType *      Os_AlarmConfigs;                         /**< config array for alarms  */
#endif

#ifdef OS_COUNTER_COUNT
Os_CounterControlType           Os_CounterControls     [OS_COUNTER_COUNT]; /**< control array for counters */
const Os_CounterConfigType *    Os_CounterConfigs;                         /**< config array for counters, NULL for defaults */

/** attributes of counters when the configuration has none */
static const Os_CounterConfigType Os_CounterDefault = {
    OS_MAXALLOWEDVALUE,
    OS_TICKSPERBASE,
    OS_MINCYCLE,
};
#endif

static Os_StatusType Os_Schedule_Internal(void);
static Os_StatusType Os_ChainTask_Internal(Os_TaskType task);
static Os_StatusType Os_TerminateTask_Internal(void);
static Os_StatusType Os_ActivateTask_Internal(Os_TaskType task);
static Os_StatusType Os_GetResource_Internal(Os_ResourceType res);
static Os_StatusType Os_ReleaseResource_Internal(Os_ResourceType res);
static Os_StatusType Os_IncrementCounter_Internal(Os_CounterType counter);

static void Os_AlarmTick   (Os_AlarmType queue[], Os_TickType ticks);
#ifdef OS_ALARM_COUNT
Os_StatusType Os_SetRelAlarm_Internal(Os_AlarmType alarm, Os_TickType increment, Os_TickType cycle);
#endif
#if(OS_EVENTS_ENABLE)
Os_StatusType Os_SetEvent_Internal(Os_TaskType task, Os_EventMaskType mask);
#endif
static void Os_AlarmAdd    (Os_AlarmType queue[], Os_AlarmType alarm);

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

#pragma INLINE
/**
 * @brief Attributes of a counter
 */
static __inline const Os_CounterConfigType* Os_CounterConfig(Os_CounterType counter)
{
    if (Os_CounterConfigs) {
        return &Os_CounterConfigs[counter];
    }
    return &Os_CounterDefault;
}

/**
 * @brief Add two tick values modulo the counter's range, maxallowedvalue + 1
 */
static Os_TickType Os_CounterAdd(Os_CounterType counter, Os_TickType lh, Os_TickType rh)
{
    uint32 range = (uint32)Os_CounterConfig(counter)->maxallowedvalue + 1u;
    return (Os_TickType)(((uint32)lh + (uint32)rh) % range);
}

/**
 * @brief Subtract two tick values modulo the counter's range, maxallowedvalue + 1
 */
static Os_TickType Os_CounterSub(Os_CounterType counter, Os_TickType lh, Os_TickType rh)
{
    uint32 range = (uint32)Os_CounterConfig(counter)->maxallowedvalue + 1u;
    return (Os_TickType)(((uint32)lh + range - ((uint32)rh % range)) % range);
}

/**
 * @brief Check a cycle against the counter's limits
 */
static boolean Os_CounterCycleValid(Os_CounterType counter, Os_TickType cycle)
{
    const Os_CounterConfigType* cfg = Os_CounterConfig(counter);
    return (cycle == 0u)
        || ((cycle >= cfg->mincycle) && (cycle <= cfg->maxallowedvalue));
}

/**
 * @brief Number of ticks after the next one until the alarm expires
 *
 * Alarms are ordered by how far ahead of their counter they expire, so the
 * whole counter range can be used. An alarm expiring at the next tick has
 * distance 0, and one expiring at the current counter value has the largest
 * distance, since it will only be reached again after the counter wraps.
 */
static Os_TickType Os_AlarmDistance(Os_AlarmType alarm)
{
    Os_CounterType counter = Os_AlarmConfigs[alarm].counter;
    return Os_CounterSub(counter
                       , Os_AlarmTicks[alarm]
                       , Os_CounterAdd(counter, Os_CounterControls[counter].ticks, 1u));
}

#pragma INLINE
/**
 * @brief Check if alarm lh expires before alarm rh, both on the same counter
 */
static __inline boolean Os_AlarmBefore(Os_AlarmType lh, Os_AlarmType rh)
{
    return Os_AlarmDistance(lh) < Os_AlarmDistance(rh);
}

/**
 * @brief Restore heap order by moving an element down below earlier children
 */
void Os_AlarmHeapify(Os_AlarmType queue[]
                   , Os_AlarmQueueIndexType index)
{
    unsigned int  child;
    unsigned int  first;
    Os_AlarmType  swap;

    while (1u) {
        /* swap with the earliest child, if it is earlier */
        first = index;
        child = (unsigned int)index * 2u;
        if ((child <= queue[0u]) && Os_AlarmBefore(queue[child], queue[first]) ) {
            first = child;
        }

        child++;
        if ((child <= queue[0u]) && Os_AlarmBefore(queue[child], queue[first]) ) {
            first = child;
        }

        if (first == index) {
            break;
        }

        swap         = queue[index];
        queue[index] = queue[first];
        queue[first] = swap;
        index        = (Os_AlarmQueueIndexType)first;
    }
}

/**
 * @brief Restore heap order by moving an element up above later parents
 */
static void Os_AlarmSiftUp(Os_AlarmType queue[]
                         , Os_AlarmQueueIndexType index)
{
    Os_AlarmType alarm = queue[index];

    while (index > 1u && Os_AlarmBefore(alarm, queue[index / 2u]) ) {
        queue[index] = queue[index / 2u];
        index       /= 2u;
    }
    queue[index] = alarm;
}

/**
 * @brief Pop the first alarm out of the queue
 * @param queue the queue to look through
 * @param alarm the resulting alarm
 */
static void Os_AlarmPop(Os_AlarmType queue[], Os_AlarmType* alarm)
{
    *alarm      = queue[1u];

    if (queue[0u] > 1u) {
        queue[1u] = queue[queue[0u]];
        queue[0u]--;
        Os_AlarmHeapify(queue, 1u);
    } else {
        queue[0u] = 0u;
    }
    Os_AlarmQueued[*alarm] = FALSE;
}

/**
 * @brief Expire the alarms of a counter that is about to reach the given tick
 * @param[in,out] queue alarm queue of the counter
 * @param[in]     ticks the counter's next value
 *
 * Must be called before the counter is advanced, since the queue is ordered
 * relative to the counter's current value. Cyclic alarms are added back
 * at least one tick later, so they do not expire twice.
 */
void Os_AlarmTick(Os_AlarmType queue[], Os_TickType ticks)
{
    /* trigger and consume any expired */
    while (queue[0] > 0u && Os_AlarmTicks[queue[1]] == ticks) {
        Os_AlarmType alarm;
        Os_AlarmPop(queue, &alarm);

        /* activate linked task, or set its events */
        if (Os_AlarmConfigs[alarm].task != OS_INVALID_TASK) {
#if(OS_EVENTS_ENABLE)
            if (Os_AlarmConfigs[alarm].event) {
                (void)Os_SetEvent_Internal(Os_AlarmConfigs[alarm].task, Os_AlarmConfigs[alarm].event);
            } else
#endif
            {
                (void)Os_ActivateTask_Internal(Os_AlarmConfigs[alarm].task);
            }
        }

        /* call any callback */
        if (Os_AlarmConfigs[alarm].callback) {
            Os_AlarmConfigs[alarm].callback();
        }

        /* readd cyclic */
        if (Os_AlarmCycles[alarm]) {
            Os_AlarmTicks[alarm] = Os_CounterAdd(Os_AlarmConfigs[alarm].counter
                                               , Os_AlarmTicks[alarm]
                                               , Os_AlarmCycles[alarm]);
            Os_AlarmAdd(queue, alarm);
        }
    }
}

void Os_AlarmAdd(Os_AlarmType queue[], Os_AlarmType alarm)
{
    queue[0]++;
    queue[queue[0u]] = alarm;
    Os_AlarmSiftUp(queue, queue[0u]);
    Os_AlarmQueued[alarm] = TRUE;
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
    Os_TaskControls[task].priority = -1;
}

/**
 * @brief Peeks into the ready lists for a task with higher or equal priority to given
 * @param[in]  min_priority minimal task priority to find
 * @param[out] task the found task. will be Os_TaskIdNone if none was found
 */
static __inline void Os_TaskPeek(Os_PriorityType min_priority, Os_TaskType* task)
{
    Os_PriorityType prio;
    *task = OS_INVALID_TASK;
    for(prio = OS_PRIO_COUNT - 1; (prio > min_priority) && (*task == OS_INVALID_TASK); --prio) {
        *task = Os_TaskReady[prio].head;
    }
}

/**
 * @brief Initializes a resource structure
 * @param res resource to initialize
 */
static void Os_ResourceInit(Os_ResourceType res)
{
    memset(&Os_ResourceControls[res], 0, sizeof(Os_ResourceControls[res]));
    Os_ResourceControls[res].next = OS_INVALID_RESOURCE;
#if(OS_ERROR_EXT_ENABLE)
    Os_ResourceControls[res].task = OS_INVALID_TASK;
#endif
}

/**
 * @brief Remove the running task's most recently taken resource
 * @param res the resource, must be the last one the running task took
 *
 * Restores the priority the task had before taking it, without rescheduling.
 */
static void Os_ResourceUnlink(Os_ResourceType res)
{
#if(OS_ERROR_EXT_ENABLE)
    Os_ResourceControls[res].task  = OS_INVALID_TASK;
#endif
    Os_TaskControls[Os_ActiveTask].resource = Os_ResourceControls[res].next;
    Os_TaskControls[Os_ActiveTask].priority = Os_ResourceControls[res].priority;
    Os_ResourceControls[res].next  = OS_INVALID_RESOURCE;
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
    OS_CHECK_EXT(Os_ActiveTask  != OS_INVALID_TASK, E_OS_STATE);

    res = Os_TaskConfigs[Os_ActiveTask].resource;
    if (res != OS_INVALID_RESOURCE && Os_TaskControls[Os_ActiveTask].resource == res) {
        /* no rescheduling here, the scheduler would just take it back, the
         * service releasing it schedules itself */
        Os_ResourceUnlink(res);
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

    if (!Os_TaskRunning()) {
        return;
    }

    res = Os_TaskConfigs[Os_ActiveTask].resource;
    if (res != OS_INVALID_RESOURCE && Os_TaskControls[Os_ActiveTask].resource == OS_INVALID_RESOURCE) {
        (void)Os_GetResource_Internal(res);
    }
}

/**
 * @brief Perform the state transition from running to suspended for a task
 * @param task task to transition to suspended state
 */
static __inline void Os_State_Running_To_Suspended(Os_TaskType task)
{
    OS_CHECK_EXT(Os_TaskControls[task].state == OS_TASK_RUNNING, E_OS_STATE);

    Os_TaskControls[task].state    = OS_TASK_SUSPENDED;
    Os_TaskControls[task].priority = -1;

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
    Os_PriorityType prio;

    OS_CHECK_EXT(Os_TaskControls[task].state == OS_TASK_RUNNING, E_OS_STATE);

    prio = Os_TaskControls[task].priority;

    Os_ReadyListPushHead(&Os_TaskReady[prio], task);
    Os_TaskControls[task].state = OS_TASK_READY;

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
    Os_PriorityType prio;

    OS_CHECK_EXT(Os_TaskControls[task].state == OS_TASK_SUSPENDED, E_OS_STATE);

    prio = Os_TaskConfigs[task].priority;
    Os_TaskControls[task].state    = OS_TASK_READY_FIRST;
    Os_TaskControls[task].priority = prio;
#if(OS_EVENTS_ENABLE)
    Os_TaskControls[task].events   = 0u; /* events are cleared on activation */
    Os_TaskControls[task].waiting  = 0u;
#endif

    Os_ReadyListPushTail(&Os_TaskReady[prio], task);
}

#if(OS_EVENTS_ENABLE)
/**
 * @brief Perform the state transition from running to waiting
 * @param task task to transition to the waiting state
 */
static __inline void Os_State_Running_To_Waiting(Os_TaskType task)
{
    OS_CHECK_EXT(Os_TaskControls[task].state == OS_TASK_RUNNING, E_OS_STATE);

    Os_TaskControls[task].state    = OS_TASK_WAITING;
    Os_TaskControls[task].priority = Os_TaskConfigs[task].priority;

    OS_POSTTASKHOOK(task);
}

/**
 * @brief Perform the state transition from waiting to ready
 * @param task task to transition to the ready state
 *
 * The task continues where it waited, so it is ready but not started anew,
 * and goes last among the ready tasks of its priority.
 */
static __inline void Os_State_Waiting_To_Ready(Os_TaskType task)
{
    Os_PriorityType prio;

    OS_CHECK_EXT(Os_TaskControls[task].state == OS_TASK_WAITING, E_OS_STATE);

    prio = Os_TaskConfigs[task].priority;
    Os_TaskControls[task].state    = OS_TASK_READY;
    Os_TaskControls[task].priority = prio;
    Os_TaskControls[task].waiting  = 0u;

    Os_ReadyListPushTail(&Os_TaskReady[prio], task);
}
#endif

/**
 * @brief Perform the state transition from ready to running
 * @param task task to transition to the running state
 *
 * This will pop the task out from the ready list, set it
 * as the running task.
 */
static __inline void Os_State_Ready_To_Running(Os_TaskType task)
{
    Os_PriorityType prio;
    Os_TaskType     task2;

    OS_CHECK_EXT( Os_TaskControls[task].state == OS_TASK_READY
               || Os_TaskControls[task].state == OS_TASK_READY_FIRST, E_OS_STATE);

    prio = Os_TaskControls[task].priority;
    Os_ReadyListPopHead(&Os_TaskReady[prio], &task2);

    OS_CHECK_EXT(task2 == task, E_OS_STATE);

    if(Os_TaskControls[task].state == OS_TASK_READY_FIRST) {
        Os_Arch_PrepareState(task);
    }

    Os_TaskControls[task].state = OS_TASK_RUNNING;

    OS_PRETASKHOOK(task);
}

/**
 * @brief Start the tasks and alarms configured to start automatically in a mode
 * @param mode application mode being started
 */
static void Os_Autostart(Os_AppModeType mode)
{
    Os_TaskType  task;
    Os_AlarmType alarm;

    for (task = 0u; task < OS_TASK_COUNT; ++task) {
        if (Os_TaskConfigs[task].autostart & OS_APPMODE_MASK(mode)) {
#if( (OS_CONFORMANCE == OS_CONFORMANCE_ECC2) ||  (OS_CONFORMANCE == OS_CONFORMANCE_BCC2) )
            Os_TaskControls[task].activation = 1;
#endif
            Os_State_Suspended_To_Ready(task);
        }
    }

#ifdef OS_ALARM_COUNT
    for (alarm = 0u; alarm < OS_ALARM_COUNT; ++alarm) {
        if (Os_AlarmConfigs[alarm].autostart & OS_APPMODE_MASK(mode)) {
            (void)Os_SetRelAlarm_Internal(alarm
                                        , Os_AlarmConfigs[alarm].alarmtime
                                        , Os_AlarmConfigs[alarm].cycletime);
        }
    }
#endif
}

/**
 * @brief Start the operating system
 * @param mode application mode to start in, OS_DEFAULTAPPMODE unless the
 *             application defines more
 *
 * Starts the tasks and alarms configured for the mode, calls the startup
 * hook and starts scheduling. Returns once the OS has been shut down, or
 * at once if the configuration or mode is invalid.
 */
void Os_Start(Os_AppModeType mode)
{
    if (!Os_Initialized) {
        return;
    }

    if (mode >= OS_APPMODE_COUNT) {
        Os_Error.service   = OSServiceId_None;
        Os_Error.status    = E_OS_VALUE;
        Os_Error.params[0] = mode;
        OS_ERRORHOOK(E_OS_VALUE);
        return;
    }

    Os_AppMode = mode;
    Os_Autostart(mode);

    /* after initialisation, before any task runs, interrupts still disabled */
    OS_STARTUPHOOK();
    if (!Os_Continue) {
        return; /* shut down from the startup hook */
    }

    Os_CallContext = OS_CONTEXT_TASK;
    Os_Schedule_Internal();
    Os_Arch_Start();
    while(Os_Continue) {
        Os_Arch_Wait();
    }
}

/**
 * @brief Shut down the operating system
 * @param error reason for the shutdown
 * @return E_OK
 *
 * Call contexts: TASK, ISR2, ERRORHOOK, STARTUPHOOK
 */
Os_StatusType Os_Shutdown_Internal(Os_StatusType error)
{
    (void)error;
    OS_SHUTDOWNHOOK(error);

    Os_Continue = FALSE;

    if (Os_TaskRunning()) {
        /* put preempted task as first ready */
        Os_State_Running_To_Ready(Os_ActiveTask);
    }

    return E_OK;
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
    Os_TaskType     task;

    if (Os_TaskRunning()) {
        prio = Os_TaskControls[Os_ActiveTask].priority;
    } else {
        prio = -1;
    }

    Os_TaskPeek(prio, &task);

    if(task != OS_INVALID_TASK) {
        if (Os_TaskRunning()) {
            /* put preempted task as first ready */
            Os_State_Running_To_Ready(Os_ActiveTask);
        }

        /* pop this task out from ready */
        Os_State_Ready_To_Running(task);
        Os_ActiveTask = task;
    }

    /* the running task holds its internal resource, however it got to run */
    if (Os_TaskRunning()) {
        Os_TaskInternalResource_Get();
    }
    return E_OK;
}

void Os_Isr(void)
{
    Os_CallContext = OS_CONTEXT_ISR1;
    Os_IncrementCounter_Internal(0u);
    Os_Schedule_Internal();
    Os_CallContext = OS_CONTEXT_TASK;
}

#if(OS_EVENTS_ENABLE)
/**
 * @brief Wait until any of the given events is set for the calling task
 * @param mask events to wait for
 * @return
 *  - E_OK on success
 *  - E_OS_ACCESS the calling task is not an extended task
 *  - E_OS_RESOURCE the calling task holds a resource other than its internal one
 *  - E_OS_CALLEVEL called from interrupt level
 *
 * If none of the events is set the task waits, and the next ready task
 * runs. The task's internal resource is released while it waits.
 *
 * Call contexts: TASK
 */
Os_StatusType Os_WaitEvent_Internal(Os_EventMaskType mask)
{
    OS_CHECK_EXT_R(Os_CallContext == OS_CONTEXT_TASK                              , E_OS_CALLEVEL);
    OS_CHECK_EXT_R(Os_TaskRunning()                                               , E_OS_CALLEVEL);
    OS_CHECK_EXT_R(Os_TaskConfigs[Os_ActiveTask].extended                         , E_OS_ACCESS);
    OS_CHECK_EXT_R(Os_TaskControls[Os_ActiveTask].resource == OS_INVALID_RESOURCE , E_OS_RESOURCE);

    if ((Os_TaskControls[Os_ActiveTask].events & mask) == 0u) {
        Os_TaskControls[Os_ActiveTask].waiting = mask;
        Os_State_Running_To_Waiting(Os_ActiveTask);
        return Os_Schedule_Internal();
    }
    return E_OK;

OS_ERRORCHECK_EXIT_POINT:
    Os_Error.service   = OSServiceId_WaitEvent;
    Os_Error.params[0] = mask;
    OS_ERRORHOOK(Os_Error.status);
    return Os_Error.status;
}

/**
 * @brief Set events for a task
 * @param task task to set the events for
 * @param mask events to set
 * @return
 *  - E_OK on success
 *  - E_OS_ID the task is invalid
 *  - E_OS_ACCESS the task is not an extended task
 *  - E_OS_STATE the task is suspended
 *
 * A task waiting for any of the events becomes ready, and may preempt the
 * caller.
 *
 * Call contexts: TASK, ISR2
 */
Os_StatusType Os_SetEvent_Internal(Os_TaskType task, Os_EventMaskType mask)
{
    OS_CHECK_EXT_R(task < OS_TASK_COUNT                                           , E_OS_ID);
    OS_CHECK_EXT_R(Os_TaskConfigs[task].extended                                  , E_OS_ACCESS);
    OS_CHECK_EXT_R(Os_TaskControls[task].state != OS_TASK_SUSPENDED               , E_OS_STATE);

    Os_TaskControls[task].events |= mask;
    if (  (Os_TaskControls[task].state == OS_TASK_WAITING)
       && (Os_TaskControls[task].events & Os_TaskControls[task].waiting)) {
        Os_State_Waiting_To_Ready(task);
        return Os_Schedule_Internal();
    }
    return E_OK;

OS_ERRORCHECK_EXIT_POINT:
    Os_Error.service   = OSServiceId_SetEvent;
    Os_Error.params[0] = task;
    Os_Error.params[1] = mask;
    OS_ERRORHOOK(Os_Error.status);
    return Os_Error.status;
}

/**
 * @brief Clear events of the calling task
 * @param mask events to clear
 * @return
 *  - E_OK on success
 *  - E_OS_ACCESS the calling task is not an extended task
 *  - E_OS_CALLEVEL called from interrupt level
 *
 * Call contexts: TASK
 */
Os_StatusType Os_ClearEvent_Internal(Os_EventMaskType mask)
{
    OS_CHECK_EXT_R(Os_CallContext == OS_CONTEXT_TASK                              , E_OS_CALLEVEL);
    OS_CHECK_EXT_R(Os_TaskRunning()                                               , E_OS_CALLEVEL);
    OS_CHECK_EXT_R(Os_TaskConfigs[Os_ActiveTask].extended                         , E_OS_ACCESS);

    Os_TaskControls[Os_ActiveTask].events &= (Os_EventMaskType)~mask;
    return E_OK;

OS_ERRORCHECK_EXIT_POINT:
    Os_Error.service   = OSServiceId_ClearEvent;
    Os_Error.params[0] = mask;
    OS_ERRORHOOK(Os_Error.status);
    return Os_Error.status;
}

/**
 * @brief Get the events set for a task
 * @param[in]  task task to query
 * @param[out] mask events set for the task
 * @return
 *  - E_OK on success
 *  - E_OS_ID the task is invalid
 *  - E_OS_ACCESS the task is not an extended task
 *  - E_OS_STATE the task is suspended
 *
 * Call contexts: TASK, ISR2, ERRORHOOK, PRETASKHOOK, POSTTASKHOOK
 */
Os_StatusType Os_GetEvent_Internal(Os_TaskType task, Os_EventMaskType* mask)
{
    OS_CHECK_EXT_R(task < OS_TASK_COUNT                                           , E_OS_ID);
    OS_CHECK_EXT_R(Os_TaskConfigs[task].extended                                  , E_OS_ACCESS);
    OS_CHECK_EXT_R(Os_TaskControls[task].state != OS_TASK_SUSPENDED               , E_OS_STATE);

    *mask = Os_TaskControls[task].events;
    return E_OK;

OS_ERRORCHECK_EXIT_POINT:
    Os_Error.service   = OSServiceId_GetEvent;
    Os_Error.params[0] = task;
    OS_ERRORHOOK(Os_Error.status);
    return Os_Error.status;
}
#endif /* OS_EVENTS_ENABLE */

/**
 * @brief Get the state of a task
 * @param[in]  task  task to query
 * @param[out] state state of the task
 * @return
 *  - E_OK on success
 *  - E_OS_ID if the task is invalid
 *
 * A task that has been activated but not started yet is reported as ready.
 *
 * Call contexts: TASK, ISR2, ERRORHOOK, PRETASKHOOK, POSTTASKHOOK
 */
Os_StatusType Os_GetTaskState_Internal(Os_TaskType task, Os_TaskStateType* state)
{
    OS_CHECK_EXT_R(task < OS_TASK_COUNT, E_OS_ID);

    if (Os_TaskControls[task].state == OS_TASK_READY_FIRST) {
        *state = OS_TASK_READY;
    } else {
        *state = Os_TaskControls[task].state;
    }
    return E_OK;

OS_ERRORCHECK_EXIT_POINT:
    Os_Error.service   = OSServiceId_GetTaskState;
    Os_Error.params[0] = task;
    OS_ERRORHOOK(Os_Error.status);
    return Os_Error.status;
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
    OS_CHECK_EXT_R(Os_TaskRunning()                                               , E_OS_STATE);
    OS_CHECK_EXT_R(Os_CallContext == OS_CONTEXT_TASK                              , E_OS_CALLEVEL);
    OS_CHECK_EXT_R(Os_TaskControls[Os_ActiveTask].resource == OS_INVALID_RESOURCE , E_OS_RESOURCE);

    Os_State_Running_To_Suspended(Os_ActiveTask);

#if( (OS_CONFORMANCE == OS_CONFORMANCE_ECC2) ||  (OS_CONFORMANCE == OS_CONFORMANCE_BCC2) )
    Os_TaskControls[Os_ActiveTask].activation--;
    if (Os_TaskControls[Os_ActiveTask].activation) {
        Os_State_Suspended_To_Ready(Os_ActiveTask);
    }
#endif

    return Os_Schedule_Internal();

OS_ERRORCHECK_EXIT_POINT:
    Os_Error.service = OSServiceId_TerminateTask;
    OS_ERRORHOOK(Os_Error.status);
    return Os_Error.status;
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
    OS_CHECK_EXT_R(Os_TaskRunning()                                               , E_OS_STATE);
    OS_CHECK_EXT_R(Os_CallContext == OS_CONTEXT_TASK                              , E_OS_CALLEVEL);
    OS_CHECK_EXT_R(Os_TaskControls[Os_ActiveTask].resource == OS_INVALID_RESOURCE , E_OS_RESOURCE);
    OS_CHECK_EXT_R(task < OS_TASK_COUNT                                           , E_OS_ID);

    /* check the activation limit before terminating, chaining to
     * itself never exceeds it since the caller's activation ends */
#if( (OS_CONFORMANCE == OS_CONFORMANCE_ECC2) ||  (OS_CONFORMANCE == OS_CONFORMANCE_BCC2) )
    OS_CHECK_R    (  (task == Os_ActiveTask)
                  || (Os_TaskControls[task].activation < Os_TaskConfigs[task].activation), E_OS_LIMIT);
#else
    OS_CHECK_R    (  (task == Os_ActiveTask)
                  || (Os_TaskControls[task].state == OS_TASK_SUSPENDED), E_OS_LIMIT);
#endif

    Os_State_Running_To_Suspended(Os_ActiveTask);

#if( (OS_CONFORMANCE == OS_CONFORMANCE_ECC2) ||  (OS_CONFORMANCE == OS_CONFORMANCE_BCC2) )
    Os_TaskControls[Os_ActiveTask].activation--;
    if (Os_TaskControls[Os_ActiveTask].activation > 0u) {
        Os_State_Suspended_To_Ready(Os_ActiveTask);
    }

    Os_TaskControls[task].activation++;
    if (Os_TaskControls[task].activation == 1u) {
        Os_State_Suspended_To_Ready(task);
    }
#else
    Os_State_Suspended_To_Ready(task);
#endif

    return Os_Schedule_Internal();

OS_ERRORCHECK_EXIT_POINT:
    Os_Error.service   = OSServiceId_ChainTask;
    Os_Error.params[0] = task;
    OS_ERRORHOOK(Os_Error.status);
    return Os_Error.status;
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
    OS_CHECK_R    (Os_TaskControls[task].state == OS_TASK_SUSPENDED, E_OS_LIMIT);
    Os_State_Suspended_To_Ready(task);
#endif
    return Os_Schedule_Internal();

OS_ERRORCHECK_EXIT_POINT:
    Os_Error.service   = OSServiceId_ActivateTask;
    Os_Error.params[0] = task;
    OS_ERRORHOOK(Os_Error.status);
    return Os_Error.status;
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
static Os_StatusType Os_GetResource_Internal(Os_ResourceType res)
{
    OS_CHECK_EXT_R(res < OS_RES_COUNT                                               , E_OS_ID);
    OS_CHECK_EXT_R(Os_TaskConfigs[Os_ActiveTask].priority <= Os_ResourceConfigs[res].priority, E_OS_ACCESS);

#if(OS_ERROR_EXT_ENABLE)
    OS_CHECK_EXT_R(Os_ResourceControls[res].task == OS_INVALID_TASK                 , E_OS_ACCESS);
    Os_ResourceControls[res].task            = Os_ActiveTask;
#endif

    /* resources may nest in any order, so never lower the priority and
     * remember what to go back to on release */
    Os_ResourceControls[res].next            = Os_TaskControls[Os_ActiveTask].resource;
    Os_ResourceControls[res].priority        = Os_TaskControls[Os_ActiveTask].priority;
    Os_TaskControls[Os_ActiveTask].resource  = res;
    if (Os_TaskControls[Os_ActiveTask].priority < Os_ResourceConfigs[res].priority) {
        Os_TaskControls[Os_ActiveTask].priority = Os_ResourceConfigs[res].priority;
    }

    return E_OK;

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
Os_StatusType Os_ReleaseResource_Internal(Os_ResourceType res)
{
    OS_CHECK_EXT_R(res < OS_RES_COUNT                             , E_OS_ID);
    OS_CHECK_EXT_R(Os_TaskControls[Os_ActiveTask].resource == res , E_OS_NOFUNC);
    OS_CHECK_EXT_R(Os_TaskConfigs[Os_ActiveTask].priority <= Os_ResourceConfigs[res].priority, E_OS_ACCESS);

#if(OS_ERROR_EXT_ENABLE)
    OS_CHECK_EXT_R(Os_ResourceControls[res].task == Os_ActiveTask , E_OS_NOFUNC);
#endif

    Os_ResourceUnlink(res);
    return Os_Schedule_Internal();

OS_ERRORCHECK_EXIT_POINT:
    Os_Error.service   = OSServiceId_ReleaseResource;
    Os_Error.params[0] = res;
    OS_ERRORHOOK(Os_Error.status);
    return Os_Error.status;
}

#ifdef OS_ALARM_COUNT

/**
 * @brief Initialize the alarm context
 * @param alarm alarm to initialize
 */
void Os_AlarmInit(Os_AlarmType alarm)
{
    Os_AlarmCycles[alarm]  = 0u;
    Os_AlarmTicks [alarm]  = 0u;
    Os_AlarmQueued[alarm] = FALSE;
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
    OS_CHECK_R    (increment != 0u                    , E_OS_VALUE); /**< @req SWS_Os_00304 */
    OS_CHECK_EXT_R(increment <= Os_CounterConfig(Os_AlarmConfigs[alarm].counter)->maxallowedvalue, E_OS_VALUE);
    OS_CHECK_EXT_R(Os_CounterCycleValid(Os_AlarmConfigs[alarm].counter, cycle), E_OS_VALUE);
    OS_CHECK_R    (Os_AlarmQueued[alarm] == FALSE, E_OS_STATE);

    Os_AlarmCycles[alarm] = cycle;
    Os_AlarmTicks [alarm] = Os_CounterAdd(Os_AlarmConfigs[alarm].counter
                                        , Os_CounterControls[Os_AlarmConfigs[alarm].counter].ticks
                                        , increment);
    Os_AlarmAdd( Os_CounterControls[Os_AlarmConfigs[alarm].counter].queue
              ,  alarm);
    return E_OK;

OS_ERRORCHECK_EXIT_POINT:
    Os_Error.service   = OSServiceId_SetRelAlarm;
    Os_Error.params[0] = alarm;
    Os_Error.params[1] = increment;
    Os_Error.params[2] = cycle;
    OS_ERRORHOOK(Os_Error.status);
    return Os_Error.status;
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
    OS_CHECK_EXT_R(start <= Os_CounterConfig(Os_AlarmConfigs[alarm].counter)->maxallowedvalue, E_OS_VALUE);
    OS_CHECK_EXT_R(Os_CounterCycleValid(Os_AlarmConfigs[alarm].counter, cycle), E_OS_VALUE);
    OS_CHECK_R    (Os_AlarmQueued[alarm] == FALSE, E_OS_STATE);

    Os_AlarmCycles[alarm] = cycle;
    Os_AlarmTicks [alarm] = start;
    Os_AlarmAdd( Os_CounterControls[Os_AlarmConfigs[alarm].counter].queue
              ,  alarm);
   return E_OK;

OS_ERRORCHECK_EXIT_POINT:
    Os_Error.service   = OSServiceId_SetAbsAlarm;
    Os_Error.params[0] = alarm;
    Os_Error.params[1] = start;
    Os_Error.params[2] = cycle;
    OS_ERRORHOOK(Os_Error.status);
    return Os_Error.status;
}

/**
 * @brief Cancels a running alarm
 * @param alarm Reference to an alarm
 * @return
 *  - E_OS_NOFUNC Alarm <AlarmID> not in use
 *  - E_OS_ID     Alarm <AlarmID> is invalid
 *
 * Call contexts: TASK, ISR2
 *
 * O(n)+O(log n)
 */
Os_StatusType Os_CancelAlarm_Internal(Os_AlarmType alarm)
{
    Os_AlarmQueueIndexType index;
    Os_AlarmType*          queue;

    OS_CHECK_EXT_R(alarm < OS_ALARM_COUNT            , E_OS_ID);
    OS_CHECK_R(Os_AlarmQueued[alarm] == TRUE, E_OS_NOFUNC);

    queue   = &Os_CounterControls[Os_AlarmConfigs[alarm].counter].queue[0];

    /* find the index of this node, must iterate all queue nodes */
    for (index = 1u; index <= queue[0]; ++index) {
        if (queue[index] == alarm) {
            break;
        }
    }

    /* just a defensive check here */
    OS_CHECK_R(index <= queue[0], E_OS_NOFUNC);

    /* if this is not the last item, move the last one in its place and
     * restore order, it may belong either above or below */
    if (queue[0] != index) {
        queue[index] = queue[queue[0]];
        queue[0]--;
        if ((index > 1u) && Os_AlarmBefore(queue[index], queue[index / 2u])) {
            Os_AlarmSiftUp(queue, index);
        } else {
            Os_AlarmHeapify(queue, index);
        }
    } else {
        queue[0]--;
    }
    Os_AlarmQueued[alarm] = FALSE;

    return E_OK;

OS_ERRORCHECK_EXIT_POINT:
    Os_Error.service   = OSServiceId_CancelAlarm;
    Os_Error.params[0] = alarm;
    OS_ERRORHOOK(Os_Error.status);
    return Os_Error.status;
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
    OS_CHECK_EXT_R(alarm < OS_ALARM_COUNT, E_OS_ID);
    OS_CHECK_R(Os_AlarmQueued[alarm] == TRUE, E_OS_NOFUNC);

    *tick = Os_CounterSub(Os_AlarmConfigs[alarm].counter
                        , Os_AlarmTicks[alarm]
                        , Os_CounterControls[Os_AlarmConfigs[alarm].counter].ticks);
    return E_OK;

OS_ERRORCHECK_EXIT_POINT:
    Os_Error.service   = OSServiceId_GetAlarm;
    Os_Error.params[0] = alarm;
    OS_ERRORHOOK(Os_Error.status);
    return Os_Error.status;
}

/**
 * @brief Get the attributes of the counter driving an alarm
 * @param[in]  alarm alarm to query
 * @param[out] info  attributes of the alarm's counter
 * @return
 *  - E_OK on success
 *  - E_OS_ID Alarm <AlarmID> is invalid
 *
 * Call contexts: TASK, ISR2, ERRORHOOK, PRETASKHOOK, POSTTASKHOOK
 */
Os_StatusType Os_GetAlarmBase_Internal(Os_AlarmType alarm, Os_AlarmBaseType* info)
{
    OS_CHECK_EXT_R(alarm < OS_ALARM_COUNT, E_OS_ID);

    *info = *Os_CounterConfig(Os_AlarmConfigs[alarm].counter);
    return E_OK;

OS_ERRORCHECK_EXIT_POINT:
    Os_Error.service   = OSServiceId_GetAlarmBase;
    Os_Error.params[0] = alarm;
    OS_ERRORHOOK(Os_Error.status);
    return Os_Error.status;
}

#endif /* OS_ALARM_COUNT */

#ifdef OS_COUNTER_COUNT

/**
 * @brief Initializes a counter to it's starting setup
 * @param counter Counter to initialize
 */
void Os_CounterInit(Os_CounterType counter)
{
    Os_AlarmQueueIndexType  index;
    Os_CounterControls[counter].ticks = 0u;

    for (index = 1u; index < OS_ALARM_COUNT+1u; ++index) {
        Os_CounterControls[counter].queue[index] = OS_INVALID_ALARM;
    }
    Os_CounterControls[counter].queue[0] = 0u;
}

/**
 * @brief Increment given counter by one and trigger any alarms linked to it
 * @param[in] counter Counter to increment
 */
Os_StatusType Os_IncrementCounter_Internal(Os_CounterType counter)
{
    Os_TickType next;

    OS_CHECK_EXT_R(counter < OS_COUNTER_COUNT, E_OS_VALUE);
    next = Os_CounterAdd(counter, Os_CounterControls[counter].ticks, 1u);
    Os_AlarmTick(Os_CounterControls[counter].queue, next);
    Os_CounterControls[counter].ticks = next;
    return E_OK;

OS_ERRORCHECK_EXIT_POINT:
    Os_Error.service   = OSServiceId_CounterIncrement;
    Os_Error.params[0] = counter;
    OS_ERRORHOOK(Os_Error.status);
    return Os_Error.status;
}

#endif /* OS_COUNTER_COUNT */

Os_StatusType Os_Syscall_Internal(Os_SyscallParamType* param)
{
    Os_StatusType res;
    switch (param->service) {
        case OSServiceId_Schedule: {
            Os_TaskInternalResource_Release();
            res = Os_Schedule_Internal();
            break;
        }

        case OSServiceId_TerminateTask: {
            Os_TaskInternalResource_Release();
            res = Os_TerminateTask_Internal();
            Os_TaskInternalResource_Get(); /* still running if it failed */
            break;
        }

        case OSServiceId_ActivateTask: {
            res = Os_ActivateTask_Internal(param->p1.task);
            break;
        }

        case OSServiceId_ChainTask: {
            Os_TaskInternalResource_Release();
            res = Os_ChainTask_Internal(param->p1.task);
            Os_TaskInternalResource_Get(); /* still running if it failed */
            break;
        }

        case OSServiceId_GetResource: {
            res = Os_GetResource_Internal(param->p1.resource);
            break;
        }

        case OSServiceId_ReleaseResource: {
            res = Os_ReleaseResource_Internal(param->p1.resource);
            break;
        }

#ifdef OS_ALARM_COUNT
        case OSServiceId_SetRelAlarm: {
            res = Os_SetRelAlarm_Internal(param->p1.alarm, param->p2.tick[0], param->p2.tick[1]);
            break;
        }

        case OSServiceId_SetAbsAlarm: {
            res = Os_SetAbsAlarm_Internal(param->p1.alarm, param->p2.tick[0], param->p2.tick[1]);
            break;
        }

        case OSServiceId_CancelAlarm: {
            res = Os_CancelAlarm_Internal(param->p1.alarm);
            break;
        }

        case OSServiceId_GetAlarm: {
            res = Os_GetAlarm_Internal(param->p1.alarm, param->p2.tick_ptr);
            break;
        }

        case OSServiceId_GetAlarmBase: {
            res = Os_GetAlarmBase_Internal(param->p1.alarm, param->p2.base_ptr);
            break;
        }
#endif

#ifdef OS_COUNTER_COUNT
        case OSServiceId_CounterIncrement: {
            res = Os_IncrementCounter_Internal(param->p1.counter);
            break;
        }
#endif

        case OSServiceId_Shutdown: {
            res = Os_Shutdown_Internal(param->p2.status);
            break;
        }

#if(OS_EVENTS_ENABLE)
        case OSServiceId_WaitEvent: {
            Os_TaskInternalResource_Release();
            res = Os_WaitEvent_Internal(param->p2.mask);
            Os_TaskInternalResource_Get(); /* still running if it did not wait */
            break;
        }

        case OSServiceId_SetEvent: {
            res = Os_SetEvent_Internal(param->p1.task, param->p2.mask);
            break;
        }

        case OSServiceId_ClearEvent: {
            res = Os_ClearEvent_Internal(param->p2.mask);
            break;
        }

        case OSServiceId_GetEvent: {
            res = Os_GetEvent_Internal(param->p1.task, param->p2.mask_ptr);
            break;
        }
#endif

        case OSServiceId_GetTaskState: {
            res = Os_GetTaskState_Internal(param->p1.task, param->p2.state_ptr);
            break;
        }

        default:
            res = E_NOT_OK;
            break;
    }

    return res;
}

/**
 * @brief Check the static configuration before using it
 * @return
 *  - E_OK on success
 *  - E_OS_VALUE a task priority is outside the configured priority levels,
 *               or counter attributes are inconsistent
 *  - E_OS_ID a task refers to an internal resource that does not exist,
 *            or an alarm to a counter that does not exist
 *  - E_OS_RESOURCE a task's internal resource has a ceiling below the task's priority
 */
static Os_StatusType Os_CheckConfig(void)
{
    Os_TaskType     task;
    Os_ResourceType res;
    Os_CounterType  counter;
    Os_AlarmType    alarm;

    for (task = 0u; task < OS_TASK_COUNT; ++task) {
        OS_CHECK_EXT_R(  (Os_TaskConfigs[task].priority >= 0)
                      && (Os_TaskConfigs[task].priority <  OS_PRIO_COUNT)  , E_OS_VALUE);

#if(OS_CONFORMANCE == OS_CONFORMANCE_ECC2)
        /* only basic tasks may have multiple activations */
        OS_CHECK_EXT_R(!Os_TaskConfigs[task].extended || (Os_TaskConfigs[task].activation <= 1u), E_OS_VALUE);
#endif

        res = Os_TaskConfigs[task].resource;
        if (res != OS_INVALID_RESOURCE) {
            OS_CHECK_EXT_R(res < OS_RES_COUNT                              , E_OS_ID);
            OS_CHECK_EXT_R(Os_TaskConfigs[task].priority <= Os_ResourceConfigs[res].priority, E_OS_RESOURCE);
        }
    }

    for (counter = 0u; counter < OS_COUNTER_COUNT; ++counter) {
        const Os_CounterConfigType* cfg = Os_CounterConfig(counter);
        OS_CHECK_EXT_R(cfg->maxallowedvalue >= 1u                          , E_OS_VALUE);
        OS_CHECK_EXT_R(cfg->ticksperbase    >= 1u                          , E_OS_VALUE);
        OS_CHECK_EXT_R(  (cfg->mincycle >= 1u)
                      && (cfg->mincycle <= cfg->maxallowedvalue)           , E_OS_VALUE);
    }

#ifdef OS_ALARM_COUNT
    for (alarm = 0u; alarm < OS_ALARM_COUNT; ++alarm) {
        OS_CHECK_EXT_R(Os_AlarmConfigs[alarm].counter < OS_COUNTER_COUNT   , E_OS_ID);
        if (Os_AlarmConfigs[alarm].autostart) {
            OS_CHECK_EXT_R(  (Os_AlarmConfigs[alarm].alarmtime >= 1u)
                          && (Os_AlarmConfigs[alarm].alarmtime <= Os_CounterConfig(Os_AlarmConfigs[alarm].counter)->maxallowedvalue)
                          && Os_CounterCycleValid(Os_AlarmConfigs[alarm].counter, Os_AlarmConfigs[alarm].cycletime), E_OS_VALUE);
        }
    }
#endif
    return E_OK;

OS_ERRORCHECK_EXIT_POINT:
    Os_Error.service   = OSServiceId_None;
    Os_Error.params[0] = task;
    OS_ERRORHOOK(Os_Error.status);
    return Os_Error.status;
}

/**
 * @brief Initializes OS internal structures with given config
 * @param config Configuration to use
 */
void Os_Init(const Os_ConfigType* config)
{
    Os_TaskType     task;
    Os_ResourceType res;
    Os_AlarmType    alarm;
    Os_CounterType  counter;
    uint_least8_t prio;

    Os_TaskConfigs     = *config->tasks;
    Os_ResourceConfigs = *config->resources;
    Os_AlarmConfigs    = *config->alarms;
    Os_CounterConfigs  = config->counters ? *config->counters : NULL;
    Os_Initialized     = FALSE;

    /* refuse a bad configuration before touching any state, Os_Start
     * will then not start */
    if (Os_CheckConfig() != E_OK) {
        return;
    }

    Os_CallContext     = OS_CONTEXT_NONE;
    Os_ActiveTask      = OS_INVALID_TASK;
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

#ifdef OS_ALARM_COUNT
    /* initialize alarms */
    for (alarm  = 0u; alarm  < OS_ALARM_COUNT; ++alarm) {
        Os_AlarmInit(alarm);
    }
#endif

#ifdef OS_COUNTER_COUNT
    for (counter = 0u; counter < OS_COUNTER_COUNT; ++counter) {
        Os_CounterInit(counter);
    }
#endif

    /* run arch init */
    Os_Arch_Init();

    Os_AppMode     = OS_DEFAULTAPPMODE;
    Os_Initialized = TRUE;
}


