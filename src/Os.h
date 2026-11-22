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

#ifdef OS_CFG_ARCH_UMS
#include "Os_Arch_Ums.h"
#endif

#ifdef OS_CFG_ARCH_HCS12
#include "Os_Arch_HCS12.h"
#endif

#ifdef OS_CFG_ARCH_RISCV
#include "Os_Arch_RiscV.h"
#endif

/**
 * @brief Stack space a port needs on every task stack, on top of the task's own usage.
 *
 * Covers what an interrupt or context switch pushes on the interrupted task's stack.
 * Ports where that is negligible, or where the stack is not taken from the
 * configuration, leave it at zero.
 */
#ifndef OS_ARCH_STACK_EXTRA
#define OS_ARCH_STACK_EXTRA 0u
#endif

/**
 * @brief Size of a task stack that needs @p size bytes for the task itself.
 *
 * Use this when declaring task stacks, so the configured size stays the task's
 * own need and the port adds what it requires, e.g.
 * `unsigned char task_stack[OS_STACK_SIZE(512)];`
 */
#define OS_STACK_SIZE(size) ((size) + OS_ARCH_STACK_EXTRA)

#ifndef OS_ERROR_EXT_ENABLE
#define OS_ERROR_EXT_ENABLE 1
#endif

#ifndef OS_CONFORMANCE
#define OS_CONFORMANCE OS_CONFORMANCE_ECC2
#endif

/** extended conformance classes have events */
#define OS_EVENTS_ENABLE ((OS_CONFORMANCE == OS_CONFORMANCE_ECC1) || (OS_CONFORMANCE == OS_CONFORMANCE_ECC2))

#ifndef OS_COUNTER_COUNT
#define OS_COUNTER_COUNT  (Os_CounterType)1u
#define OS_COUNTER_SYSTEM (Os_CounterType)0u
#endif

/* attributes of the system counter when the configuration has no counters */
#ifndef OS_TICKSPERBASE
#define OS_TICKSPERBASE   (Os_TickType)1u
#endif

#ifndef OS_MINCYCLE
#define OS_MINCYCLE       (Os_TickType)1u
#endif

#if defined(OS_TICK_US) && !defined(OS_TICKDURATION)
#define OS_TICKDURATION   ((uint32)OS_TICK_US * 1000u) /**< duration of a system counter tick in nanoseconds */
#endif

#ifdef __GNUC__
#define Os_Unlikely(x)  __builtin_expect((x),0)
#define Os_Likely(x)    __builtin_expect((x),1)
#else
#define Os_Unlikely(x) (x)
#define Os_Likely(x)   (x)
#endif

/**
 * @brief Structure describing a tasks static configuration
 */
typedef struct Os_TaskConfigType {
    Os_PriorityType  priority;    /**< @brief fixed priority of task */
    Os_TaskEntryType entry;       /**< @brief entry point of task */
    void*            stack;       /**< @brief bottom of stack pointer */
    size_t           stack_size;  /**< @brief how large is the stack pointed to by stack */
    Os_AppModeMaskType autostart; /**< @brief application modes the task starts automatically in,
                                   *          OS_APPMODE_MASK(mode) bits, so 1 is the default mode */
#if( (OS_CONFORMANCE == OS_CONFORMANCE_ECC2) ||  (OS_CONFORMANCE == OS_CONFORMANCE_BCC2) )
    uint8            activation;  /**< @brief maximum number of activations allowed */
#endif
    Os_ResourceType  resource;    /**< @brief internal resource of task, can be Os_TaskIdNone */
#if(OS_EVENTS_ENABLE)
    boolean          extended;    /**< @brief extended task, which can wait for events (ECC1/ECC2) */
#endif
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
#if(OS_EVENTS_ENABLE)
    Os_EventMaskType events;      /**< @brief events set for the task */
    Os_EventMaskType waiting;     /**< @brief events the task waits for */
#endif
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
    Os_PriorityType priority;     /**< @brief priority of the holder before it took the resource */
#if(OS_ERROR_EXT_ENABLE)
    Os_TaskType     task;         /**< @brief task currently holding resource */
#endif
} Os_ResourceControlType;

/**
 * @brief Structure holding configuration setup for each alarm
 */
typedef void (*Os_AlarmCallbackType)(void); /**< alarm callback, see Os_AlarmConfigType::callback */

typedef struct Os_AlarmConfigType {
    Os_TaskType     task;         /**< @brief task to activate, or OS_INVALID_TASK */
    Os_CounterType  counter;      /**< @brief counter driving this alarm */
    Os_AlarmCallbackType callback;/**< @brief called on expiry if not NULL, at interrupt level
                                   *          with interrupts disabled, so it may not call
                                   *          any service except the interrupt ones */
    Os_AppModeMaskType autostart; /**< @brief application modes the alarm starts automatically in */
    Os_TickType     alarmtime;    /**< @brief ticks from start until first expiry when autostarted */
    Os_TickType     cycletime;    /**< @brief cycle when autostarted, 0 for a single expiry */
#if(OS_EVENTS_ENABLE)
    Os_EventMaskType event;       /**< @brief if not 0, set these events for task instead of activating it (ECC1/ECC2) */
#endif
} Os_AlarmConfigType;

/**
 * @brief Counter attributes, also returned for an alarm by Os_GetAlarmBase
 */
typedef struct Os_AlarmBaseType {
    Os_TickType     maxallowedvalue; /**< @brief largest counter value, after which it wraps to 0 */
    Os_TickType     ticksperbase;    /**< @brief ticks needed to reach a counter specific unit */
    Os_TickType     mincycle;        /**< @brief smallest allowed cycle of a cyclic alarm */
} Os_AlarmBaseType;

typedef Os_AlarmBaseType Os_CounterConfigType;

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
    const Os_CounterConfigType  (*counters)[OS_COUNTER_COUNT]; /**< @brief pointer to an array of counter configurations,
                                                                *          or NULL for counters using the full tick range */
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


void       Os_Init(const Os_ConfigType* config);
void       Os_Start(Os_AppModeType mode);

extern Os_AppModeType                  Os_AppMode;

/**
 * @brief Get the application mode the OS was started in
 */
static __inline Os_AppModeType Os_GetActiveApplicationMode(void)
{
    return Os_AppMode;
}
void       Os_Isr(void);


typedef enum Os_ServiceIdType {
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
    OSServiceId_Shutdown,
    OSServiceId_GetTaskState,
    OSServiceId_GetAlarmBase,
    OSServiceId_WaitEvent,
    OSServiceId_SetEvent,
    OSServiceId_ClearEvent,
    OSServiceId_GetEvent,
} Os_ServiceIdType;

typedef struct Os_SyscallParamType {
    Os_ServiceIdType    service;
    union {
        Os_TaskType     task;
        Os_AlarmType    alarm;
        Os_CounterType  counter;
        Os_ResourceType resource;
    } p1;
    union {
        Os_TickType  tick[2];
        Os_TickType*      tick_ptr;
        Os_TaskStateType* state_ptr;
        Os_StatusType     status;
        Os_AlarmBaseType* base_ptr;
        Os_EventMaskType  mask;
        Os_EventMaskType* mask_ptr;
    } p2;
} Os_SyscallParamType;

extern Os_StatusType Os_Arch_Syscall(Os_SyscallParamType* param);

/** @copydoc Os_Schedule_Internal */
static __inline Os_StatusType Os_Schedule(void)
{
    Os_SyscallParamType param;
    param.service = OSServiceId_Schedule;
    return Os_Arch_Syscall(&param);
}

/** @copydoc Os_TerminateTask_Internal */
static __inline Os_StatusType Os_TerminateTask(void)
{
    Os_SyscallParamType param;
    param.service = OSServiceId_TerminateTask;
    return Os_Arch_Syscall(&param);
}

/** @copydoc Os_ActivateTask_Internal */
static __inline Os_StatusType Os_ActivateTask(Os_TaskType task)
{
    Os_SyscallParamType param;
    param.service = OSServiceId_ActivateTask;
    param.p1.task = task;
    return Os_Arch_Syscall(&param);
}

/** @copydoc Os_ChainTask_Internal */
static __inline Os_StatusType Os_ChainTask(Os_TaskType task)
{
    Os_SyscallParamType param;
    param.service = OSServiceId_ChainTask;
    param.p1.task = task;
    return Os_Arch_Syscall(&param);
}

/** @copydoc Os_GetResource_Internal */
static __inline Os_StatusType Os_GetResource(Os_ResourceType res)
{
    Os_SyscallParamType param;
    param.service     = OSServiceId_GetResource;
    param.p1.resource = res;
    return Os_Arch_Syscall(&param);
}

/** @copydoc Os_ReleaseResource_Internal */
static __inline Os_StatusType Os_ReleaseResource(Os_ResourceType res)
{
    Os_SyscallParamType param;
    param.service     = OSServiceId_ReleaseResource;
    param.p1.resource = res;
    return Os_Arch_Syscall(&param);
}

/** @copydoc Os_SetRelAlarm_Internal */
static __inline Os_StatusType Os_SetRelAlarm(Os_AlarmType alarm, Os_TickType increment, Os_TickType cycle)
{
    Os_SyscallParamType param;
    param.service    = OSServiceId_SetRelAlarm;
    param.p1.alarm   = alarm;
    param.p2.tick[0] = increment;
    param.p2.tick[1] = cycle;
    return Os_Arch_Syscall(&param);
}

/** @copydoc Os_SetAbsAlarm_Internal */
static __inline Os_StatusType Os_SetAbsAlarm(Os_AlarmType alarm, Os_TickType start    , Os_TickType cycle)
{
    Os_SyscallParamType param;
    param.service    = OSServiceId_SetAbsAlarm;
    param.p1.alarm   = alarm;
    param.p2.tick[0] = start;
    param.p2.tick[1] = cycle;
    return Os_Arch_Syscall(&param);
}

/** @copydoc Os_CancelAlarm_Internal */
static __inline Os_StatusType Os_CancelAlarm(Os_AlarmType alarm)
{
    Os_SyscallParamType param;
    param.service    = OSServiceId_CancelAlarm;
    param.p1.alarm   = alarm;
    return Os_Arch_Syscall(&param);
}

/** @copydoc Os_GetAlarm_Internal */
static __inline Os_StatusType Os_GetAlarm   (Os_AlarmType alarm, Os_TickType* tick)
{
    Os_SyscallParamType param;
    param.service     = OSServiceId_GetAlarm;
    param.p1.alarm    = alarm;
    param.p2.tick_ptr = tick;
    return Os_Arch_Syscall(&param);
}

#if(OS_EVENTS_ENABLE)
/**
 * @brief Wait until any of the given events is set for the calling extended task
 * @param mask events to wait for
 * @return
 *  - E_OK on success
 *  - E_OS_ACCESS the calling task is not an extended task
 *  - E_OS_RESOURCE the calling task holds a resource
 *  - E_OS_CALLEVEL called from interrupt level
 */
static __inline Os_StatusType Os_WaitEvent(Os_EventMaskType mask)
{
    Os_SyscallParamType param;
    param.service  = OSServiceId_WaitEvent;
    param.p2.mask  = mask;
    return Os_Arch_Syscall(&param);
}

/**
 * @brief Set events for an extended task, waking it if it waits for any of them
 * @param task task to set the events for
 * @param mask events to set
 * @return
 *  - E_OK on success
 *  - E_OS_ID the task is invalid
 *  - E_OS_ACCESS the task is not an extended task
 *  - E_OS_STATE the task is suspended
 */
static __inline Os_StatusType Os_SetEvent(Os_TaskType task, Os_EventMaskType mask)
{
    Os_SyscallParamType param;
    param.service  = OSServiceId_SetEvent;
    param.p1.task  = task;
    param.p2.mask  = mask;
    return Os_Arch_Syscall(&param);
}

/**
 * @brief Clear events of the calling extended task
 * @param mask events to clear
 * @return
 *  - E_OK on success
 *  - E_OS_ACCESS the calling task is not an extended task
 *  - E_OS_CALLEVEL called from interrupt level
 */
static __inline Os_StatusType Os_ClearEvent(Os_EventMaskType mask)
{
    Os_SyscallParamType param;
    param.service  = OSServiceId_ClearEvent;
    param.p2.mask  = mask;
    return Os_Arch_Syscall(&param);
}

/**
 * @brief Get the events set for an extended task
 * @param[in]  task task to query
 * @param[out] mask events set for the task
 * @return
 *  - E_OK on success
 *  - E_OS_ID the task is invalid
 *  - E_OS_ACCESS the task is not an extended task
 *  - E_OS_STATE the task is suspended
 */
static __inline Os_StatusType Os_GetEvent(Os_TaskType task, Os_EventMaskType* mask)
{
    Os_SyscallParamType param;
    param.service     = OSServiceId_GetEvent;
    param.p1.task     = task;
    param.p2.mask_ptr = mask;
    return Os_Arch_Syscall(&param);
}
#endif /* OS_EVENTS_ENABLE */

/**
 * @brief Get the attributes of the counter driving an alarm
 * @param[in]  alarm alarm to query
 * @param[out] info  attributes of the alarm's counter
 * @return
 *  - E_OK on success
 *  - E_OS_ID if the alarm is invalid
 */
static __inline Os_StatusType Os_GetAlarmBase(Os_AlarmType alarm, Os_AlarmBaseType* info)
{
    Os_SyscallParamType param;
    param.service     = OSServiceId_GetAlarmBase;
    param.p1.alarm    = alarm;
    param.p2.base_ptr = info;
    return Os_Arch_Syscall(&param);
}

static __inline Os_StatusType Os_IncrementCounter(Os_CounterType counter)
{
    Os_SyscallParamType param;
    param.service    = OSServiceId_CounterIncrement;
    param.p1.counter = counter;
    return Os_Arch_Syscall(&param);
}

/**
 * @brief Shut down the operating system
 * @param error reason for the shutdown, passed to the shutdown hook
 */
static __inline Os_StatusType Os_Shutdown(Os_StatusType error)
{
    Os_SyscallParamType param;
    param.service   = OSServiceId_Shutdown;
    param.p2.status = error;
    return Os_Arch_Syscall(&param);
}

/**
 * @brief Get the state of a task
 * @param[in]  task  task to query
 * @param[out] state OS_TASK_SUSPENDED, OS_TASK_READY, OS_TASK_WAITING or OS_TASK_RUNNING
 * @return
 *  - E_OK on success
 *  - E_OS_ID if the task is invalid
 */
static __inline Os_StatusType Os_GetTaskState(Os_TaskType task, Os_TaskStateType* state)
{
    Os_SyscallParamType param;
    param.service      = OSServiceId_GetTaskState;
    param.p1.task      = task;
    param.p2.state_ptr = state;
    return Os_Arch_Syscall(&param);
}

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

Std_ReturnType Os_Syscall_Internal(Os_SyscallParamType *param);

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
extern void    Os_ErrorHook(Os_StatusType status);
extern void    Os_ErrorHookCall(Os_StatusType status);
/* services failing inside the error hook do not call it again */
#define OS_ERRORHOOK(status) Os_ErrorHookCall(status)
#else
#define OS_ERRORHOOK(status)
#endif

#ifndef OS_STARTUPHOOK_ENABLE
#define OS_STARTUPHOOK_ENABLE 0
#endif

#if(OS_STARTUPHOOK_ENABLE)
extern void Os_StartupHook(void);
#define OS_STARTUPHOOK() Os_StartupHook()
#else
#define OS_STARTUPHOOK()
#endif

#ifndef OS_SHUTDOWNHOOK_ENABLE
#define OS_SHUTDOWNHOOK_ENABLE 0
#endif

#if(OS_SHUTDOWNHOOK_ENABLE)
extern void Os_ShutdownHook(Os_StatusType error);
#define OS_SHUTDOWNHOOK(error) Os_ShutdownHook(error)
#else
#define OS_SHUTDOWNHOOK(error)
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
