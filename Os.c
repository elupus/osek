

#include <stdint.h>
#include "Os_Types.h"
#include "Os.h"

Os_TaskControlType              Os_TaskControls        [OS_TASK_COUNT];
Os_ReadyListType                Os_TaskReady           [OS_PRIO_COUNT];
Os_TaskType                     Os_TaskRunning;
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

void Os_TaskPrepare(Os_TaskType task)
{
    /* TODO - prepare task for first execution */
}

void Os_TaskSetReady(Os_TaskType task)
{
    Os_PriorityType prio = Os_TaskConfigs[task].priority;
    Os_ReadyListPushTail(&Os_TaskReady[prio], task);
}

void Os_TaskPop(Os_PriorityType min_priority, Os_TaskType* task)
{
    Os_PriorityType prio = OS_PRIO_COUNT - 1;
    do {
        Os_ReadyListPopHead(&Os_TaskReady[prio], task);
    } while (prio > min_priority && *task == Os_TaskIdNone);
}

void Os_TaskSwitch(Os_TaskType task)
{
    Os_TaskType prev = Os_TaskRunning;
    if (prev == task) {
        return;
    }

    if (prev != Os_TaskIdNone) {
        Os_TaskSetReady(prev);
    }
    Os_TaskRunning = task;

    /* store state */

    if (Os_TaskRunning == prev) {
        /* we have an restored state */
    } else {
        /* restore state */
    }
}

void Os_Start(void)
{

}

StatusType Os_Schedule(void)
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

        if (task == Os_TaskIdNone && Os_TaskRunning == Os_TaskIdNone) {
            continue;
            /* NOP - continue */
        }

        /* no direct return if called from task
         * if called from ISR, this will just prepare next task */
        Os_TaskSwitch(task);
        break;
    }
    return E_OK;
}

StatusType Os_TerminateTask(void)
{
    Os_TaskControls[Os_TaskRunning].active--;
    if (Os_TaskControls[Os_TaskRunning].active) {
        Os_TaskPrepare(Os_TaskRunning);
        Os_TaskSetReady(Os_TaskRunning);
    }
    Os_TaskRunning = Os_TaskIdNone;
    return Os_Schedule();
}

StatusType Os_ActivateTask(Os_TaskType task)
{
    Os_TaskControls[task].active++;
    if (Os_TaskControls[task].active == 1u) {
        Os_TaskPrepare(task);
        Os_TaskSetReady(task);
    }
    return Os_Schedule();
}

void Os_Init(const Os_ConfigType* config)
{
    Os_TaskType task;
    uint_least8_t prio;

    Os_TaskConfigs = *config->tasks;

    for (prio = 0u; prio < OS_PRIO_COUNT; ++prio) {
        Os_ReadyListInit(Os_TaskReady);
    }

    for (task = 0u; task < OS_TASK_COUNT; ++task) {
        Os_ActivateTask(task);
    }
    Os_TaskPop(0u, &task);
    Os_TaskRunning = Os_TaskIdNone;
}


