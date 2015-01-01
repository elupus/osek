

#include <stdint.h>
#include <string.h>

#include "Os_Types.h"
#include "Os.h"

Os_TaskControlType              Os_TaskControls        [OS_TASK_COUNT];
Os_ReadyListType                Os_TaskReady           [OS_PRIO_COUNT];
Os_TaskType                     Os_TaskRunning;
Os_ContextType                  Os_CallContext;
const Os_TaskConfigType *       Os_TaskConfigs;

void Os_ReadyListPushHead(Os_ReadyListType* list, Os_TaskType task)
{
    Os_TaskControls[task].next = list->head;
    list->head = task;
    if (list->tail == Os_TaskIdNone) {
        list->tail = task;
    }
}

void Os_ReadyListPopHead(Os_ReadyListType* list, Os_TaskType* task)
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

void Os_ReadyListPushTail(Os_ReadyListType* list, Os_TaskType task)
{
    Os_TaskControls[task].next = Os_TaskIdNone;
    if (list->head == Os_TaskIdNone) {
        list->head = task;
    } else {
        Os_TaskControls[list->tail].next = task;
    }
    list->tail = task;
}

void Os_ReadyListInit(Os_ReadyListType* list)
{
    list->head = Os_TaskIdNone;
    list->tail = Os_TaskIdNone;
}

void Os_TaskInit(Os_TaskType task)
{
    memset(&Os_TaskControls[task], 0, sizeof(Os_TaskControls[task]));
}

static __inline void Os_TaskSetReadyTail(Os_TaskType task)
{
    Os_PriorityType prio = Os_TaskConfigs[task].priority;
    Os_ReadyListPushTail(&Os_TaskReady[prio], task);
}

static __inline void Os_TaskSetReadyHead(Os_TaskType task)
{
    Os_PriorityType prio = Os_TaskConfigs[task].priority;
    Os_ReadyListPushHead(&Os_TaskReady[prio], task);
}

void Os_TaskPop(Os_PriorityType min_priority, Os_TaskType* task)
{
    Os_PriorityType prio;
    *task = Os_TaskIdNone;
    for(prio = OS_PRIO_COUNT; prio > min_priority && *task == Os_TaskIdNone; --prio) {
        Os_ReadyListPopHead(&Os_TaskReady[prio-1], task);
    }
}

void Os_TaskSwitch(Os_TaskType task)
{
    Os_TaskType prev = Os_TaskRunning;

    Os_TaskRunning = task;
    if (prev != Os_TaskIdNone) {
        /* put preempted task as first ready */
        Os_TaskSetReadyHead(prev);

        /* store state */
        Os_Arch_StoreState(prev);
    }

    if (Os_TaskRunning == prev) {
        /* we have an restored state */
    } else {
        /* restore state */
        Os_Arch_RestoreState(task);
    }
}

void Os_Start(void)
{
    Os_Arch_EnableAllInterrupts();
    while(1) {
        (void)Os_Schedule();
    }
}

StatusType Os_ScheduleInternal(void)
{
    Os_PriorityType prio;
    Os_TaskType     task;
    if (Os_TaskRunning == Os_TaskIdNone) {
        prio = 0u;
    } else {
        prio = Os_TaskConfigs[Os_TaskRunning].priority + 1;
    }

    while(1) {
        Os_TaskPop(prio, &task);

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

        /* no direct return if called from task
         * if called from ISR, this will just prepare next task */
        Os_TaskSwitch(task);
        break;
    }
    return E_OK;
}

StatusType Os_Schedule(void)
{
    StatusType res;
    Os_Arch_DisableAllInterrupts();
    res = Os_ScheduleInternal();
    Os_Arch_EnableAllInterrupts();
    return res;
}

void Os_Isr(void)
{
    Os_CallContext = OS_CONTEXT_ISR;
    Os_ScheduleInternal();
    Os_CallContext = OS_CONTEXT_TASK;
}

StatusType Os_TerminateTask(void)
{
    StatusType res;
    Os_Arch_DisableAllInterrupts();
    Os_TaskControls[Os_TaskRunning].activation--;
    if (Os_TaskControls[Os_TaskRunning].activation) {
        Os_Arch_PrepareState(Os_TaskRunning);
        Os_TaskSetReadyTail(Os_TaskRunning);
    }
    Os_TaskRunning = Os_TaskIdNone;
    res = Os_ScheduleInternal();
    Os_Arch_EnableAllInterrupts();
    return res;
}

StatusType Os_ActivateTask(Os_TaskType task)
{
    StatusType res;
    Os_Arch_DisableAllInterrupts();
    Os_TaskControls[task].activation++;
    if (Os_TaskControls[task].activation == 1u) {
        Os_Arch_PrepareState(task);
        Os_TaskSetReadyTail(task);
    }
    res = Os_ScheduleInternal();
    Os_Arch_EnableAllInterrupts();
    return res;
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

    Os_Arch_Init();

    /* setup autostarting tasks */
    for (task = 0u; task < OS_TASK_COUNT; ++task) {
        Os_TaskInit(task);

        if (Os_TaskConfigs[task].autostart) {
            Os_Arch_PrepareState(task);
            Os_TaskControls[task].activation = 1;
            Os_TaskSetReadyTail(task);
        }
    }

    /* there is no task running */
    Os_TaskRunning = Os_TaskIdNone;
}


