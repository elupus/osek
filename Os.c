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

#include <string.h>

#include "Std_Types.h"
#include "Os_Types.h"
#include "Os.h"

Os_TaskControlType              Os_TaskControls        [OS_TASK_COUNT];
Os_ReadyListType                Os_TaskReady           [OS_PRIO_COUNT];
Os_TaskType                     Os_TaskRunning;
Os_ContextType                  Os_CallContext;
const Os_TaskConfigType *       Os_TaskConfigs;

const Os_ResourceConfigType *   Os_ResourceConfigs;
Os_ResourceControlType          Os_ResourceControls    [OS_RES_COUNT];

static void Os_ReadyListPushHead(Os_ReadyListType* list, Os_TaskType task)
{
    Os_TaskControls[task].next = list->head;
    list->head = task;
    if (list->tail == Os_TaskIdNone) {
        list->tail = task;
    }
}

static void Os_ReadyListPopHead(Os_ReadyListType* list, Os_TaskType* task)
{
    if (list->tail == Os_TaskIdNone) {
        *task = Os_TaskIdNone;
    } else {
        *task = list->head;
        if (list->tail == *task) {
            list->tail = Os_TaskIdNone;
            list->head = Os_TaskIdNone;
        }
        list->head = Os_TaskControls[*task].next;
    }
    Os_TaskControls[*task].next = Os_TaskIdNone;
}

static void Os_ReadyListPushTail(Os_ReadyListType* list, Os_TaskType task)
{
    Os_TaskControls[task].next = Os_TaskIdNone;
    if (list->head == Os_TaskIdNone) {
        list->head = task;
    } else {
        Os_TaskControls[list->tail].next = task;
    }
    list->tail = task;
}

static void Os_ReadyListInit(Os_ReadyListType* list)
{
    list->head = Os_TaskIdNone;
    list->tail = Os_TaskIdNone;
}

static void Os_TaskInit(Os_TaskType task)
{
    memset(&Os_TaskControls[task], 0, sizeof(Os_TaskControls[task]));
    Os_TaskControls[task].next     = Os_TaskIdNone;
    if (Os_TaskConfigs[task].autostart) {
        Os_TaskControls[task].activation = 1;
    }
}

static void Os_TaskPeek(Os_PriorityType min_priority, Os_TaskType* task)
{
    Os_PriorityType prio;
    *task = Os_TaskIdNone;
    for(prio = OS_PRIO_COUNT; prio > min_priority && *task == Os_TaskIdNone; --prio) {
        *task = Os_TaskReady[prio-1].head;
    }
}

static __inline void Os_State_Running_To_Suspended(Os_TaskType task)
{
    OS_ERRORCHECK(Os_TaskControls[task].state == OS_TASK_RUNNING, E_OS_STATE);

    Os_TaskControls[task].state = OS_TASK_SUSPENDED;
}

static __inline void Os_State_Running_To_Ready(Os_TaskType task)
{
    OS_ERRORCHECK(Os_TaskControls[task].state == OS_TASK_RUNNING, E_OS_STATE);

    Os_PriorityType prio = Os_TaskConfigs[task].priority;
    Os_ReadyListPushHead(&Os_TaskReady[prio], task);
    Os_TaskControls[task].state = OS_TASK_READY;
}

static __inline void Os_State_Suspended_To_Ready(Os_TaskType task)
{
    OS_ERRORCHECK(Os_TaskControls[task].state == OS_TASK_SUSPENDED, E_OS_STATE);

    Os_Arch_PrepareState(task);
    Os_PriorityType prio = Os_TaskConfigs[task].priority;
    Os_ReadyListPushTail(&Os_TaskReady[prio], task);
    Os_TaskControls[task].state = OS_TASK_READY;
}

static __inline void Os_State_Ready_To_Running(Os_TaskType task)
{
    OS_ERRORCHECK(Os_TaskControls[task].state == OS_TASK_READY   , E_OS_STATE);

    Os_PriorityType prio = Os_TaskConfigs[task].priority;
    Os_TaskType     task2;
    Os_ReadyListPopHead(&Os_TaskReady[prio], &task2);

    OS_ERRORCHECK(task2 == task, E_OS_STATE);

    Os_TaskControls[task].state = OS_TASK_RUNNING;
}

void Os_Start(void)
{
    Os_Arch_EnableAllInterrupts();
    while(1) {
        (void)Os_Schedule();
    }
}

StatusType Os_Schedule_Internal(void)
{
    Os_PriorityType prio;
    Os_TaskType     task, prev;
    if (Os_TaskRunning == Os_TaskIdNone) {
        prio = 0u;
    } else {
        prio = Os_TaskConfigs[Os_TaskRunning].priority + 1;
    }

    while(1) {
        Os_TaskPeek(prio, &task);

        if (task == Os_TaskIdNone) {
            if (Os_TaskRunning != Os_TaskIdNone) {
                break;    /* continue with current */
            }
            if (Os_CallContext != OS_CONTEXT_TASK) {
                break;    /* no new task, and we are inside tick, so we need to return out */
            }

            /* idling in last task */
            continue;
        }

        if (Os_TaskRunning != Os_TaskIdNone) {
            /* put preempted task as first ready */
            Os_State_Running_To_Ready(Os_TaskRunning);
        }

        /* pop this task out from ready */
        Os_State_Ready_To_Running(task);

        /* no direct return if called from task
         * if called from ISR, this will just
         * prepare next task. function can use
         * current running task to check if it
         * just restored to this state */
        prev = Os_TaskRunning;
        Os_TaskRunning = task;
        Os_Arch_SwapState(task, prev);
        break;
    }
    return E_OK;
}

void Os_Isr(void)
{
    Os_CallContext = OS_CONTEXT_ISR;
    Os_Schedule_Internal();
    Os_CallContext = OS_CONTEXT_TASK;
}


StatusType Os_TerminateTask_Internal(void)
{
    OS_ERRORCHECK_R(Os_TaskControls[Os_TaskRunning].activation > 0, E_OS_LIMIT);
    OS_ERRORCHECK_R(Os_CallContext != OS_CONTEXT_ISR              , E_OS_CALLEVEL);

    Os_State_Running_To_Suspended(Os_TaskRunning);

    Os_TaskControls[Os_TaskRunning].activation--;
    if (Os_TaskControls[Os_TaskRunning].activation) {
        Os_State_Suspended_To_Ready(Os_TaskRunning);
    }

    Os_TaskRunning = Os_TaskIdNone;
    return Os_Schedule_Internal();
}

StatusType Os_ActivateTask_Internal(Os_TaskType task)
{
    OS_ERRORCHECK_R(task < OS_TASK_COUNT                   , E_OS_ID);
    OS_ERRORCHECK_R(Os_TaskControls[task].activation <  255, E_OS_LIMIT);

    Os_TaskControls[task].activation++;
    if (Os_TaskControls[task].activation == 1u) {
        Os_State_Suspended_To_Ready(task);
    }

    return Os_Schedule_Internal();
}

StatusType Os_GetResource_Internal(Os_ResourceType res)
{
    OS_ERRORCHECK_R(res < OS_RES_COUNT, E_OS_ID);

    if (res == OS_RES_SCHEDULER) {
        Os_Arch_DisableAllInterrupts();
        result = E_OK;
    } else {
        result = E_OS_RESOURCE;
    }
    return result;
}

StatusType Os_ReleaseResource_Internal(Os_ResourceType res)
{
    OS_ERRORCHECK_R(res < OS_RES_COUNT, E_OS_ID);

    if (res == OS_RES_SCHEDULER) {
        result = E_OK;
    } else {
        result = E_OS_RESOURCE;
    }
    return result;
}


void Os_Init(const Os_ConfigType* config)
{
    Os_TaskType task;
    uint_least8_t prio;

    Os_TaskConfigs = *config->tasks;
    Os_CallContext = OS_CONTEXT_TASK;

    memset(&Os_TaskControls, 0u, sizeof(Os_TaskControls));

    for (prio = 0u; prio < OS_PRIO_COUNT; ++prio) {
        Os_ReadyListInit(&Os_TaskReady[prio]);
    }

    /* initialize task */
    for (task = 0u; task < OS_TASK_COUNT; ++task) {
        Os_TaskInit(task);
    }

    /* run arch init */
    Os_Arch_Init();

    /* make sure any activated task is in ready list */
    for (task = 0u; task < OS_TASK_COUNT; ++task) {
        if (Os_TaskControls[task].activation) {
            Os_State_Suspended_To_Ready(task);
        }
    }

    /* there is no task running */
    Os_TaskRunning = Os_TaskIdNone;
}


