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

#ifndef OS_TYPES_H_
#define OS_TYPES_H_

#include "Std_Types.h"

typedef uint8 Os_TaskType;        /**< task identifier */
typedef uint8 Os_ResourceType;    /**< resource identifier */
typedef uint8 Os_PriorityType;    /**< priority level */

typedef uint8 Os_StatusType;      /**< return value for os functions */

typedef uint8  Os_AlarmType;      /**< alarm identifier */
typedef uint16 Os_TickType;       /**< tick value identifier */

#define OS_MAXALLOWEDVALUE UINT8_MAX

#define OS_INVALID_TASK      (Os_TaskType)(-1)
#define OS_INVALID_RESOURCE  (Os_ResourceType)(-1)
#define OS_INVALID_ALARM     (Os_AlarmType)(-1)

/**
 * @brief Current call context
 */
typedef enum Os_ContextType {
    OS_CONTEXT_NONE  = 0,         /**< OS_CONTEXT_NONE - Os has not been started */
    OS_CONTEXT_TASK  = 1,         /**< OS_CONTEXT_TASK - Os currently called from a task */
    OS_CONTEXT_ISR1  = 2,         /**< OS_CONTEXT_ISR1 - Os currently called from interrupt category 1 */
    OS_CONTEXT_ISR2  = 3,         /**< OS_CONTEXT_ISR2 - Os currently called from interrupt category 2 */
} Os_ContextType;

/**
 * @brief State of a task
 */
typedef enum Os_TaskStateEnum {
    OS_TASK_SUSPENDED = 0,        /**< OS_TASK_SUSPENDED */
    OS_TASK_READY     = 1,        /**< OS_TASK_READY */
    OS_TASK_WAITING   = 2,        /**< OS_TASK_WAITING */
    OS_TASK_RUNNING   = 3,        /**< OS_TASK_RUNNING */
} Os_TaskStateEnum;

#define OS_RES_SCHEDULER (Os_ResourceType)0

typedef void          (*Os_TaskEntryType)(void); /**< type for the entry point of a task */

/**
 * @brief Structure describing a tasks static configuration
 */
typedef struct Os_TaskConfigType {
    Os_PriorityType  priority;    /**< @brief fixed priority of task */
    Os_TaskEntryType entry;       /**< @brief entry point of task */
    void*            stack;       /**< @brief bottom of stack pointer */
    size_t           stack_size;  /**< @brief how large is the stack pointed to by stack */
    boolean          autostart;   /**< @brief should this task start automatically */
    uint8            activation;  /**< @brief maximum number of activations allowed */
    Os_ResourceType  resource;    /**< @brief internal resource of task, can be Os_TaskIdNone */
} Os_TaskConfigType;

/**
 * @brief Structure holding information of the state of a task
 */
typedef struct Os_TaskControlType {
    Os_TaskStateEnum state;       /**< @brief current state */
    uint8            activation;  /**< @brief number of activations for given task */
    Os_TaskType      next;        /**< @brief next task in the same ready list */
    Os_ResourceType  resource;    /**< @brief last taken resource for task (rest is linked list */
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
    Os_TaskType     task;         /**< @brief task currently holding resource */
} Os_ResourceControlType;

/**
 * @brief Structure holding configuration setup for each alarm
 */
typedef struct Os_AlarmConfigType {
    Os_TaskType     task;         /**< @brief task to activate */
} Os_AlarmConfigType;

/**
 * @brief Structure holding configuration setup for each alarm
 */
typedef struct Os_AlarmControlType {
    Os_AlarmType    next;         /**< @brief next task scheduled */
    Os_TickType     ticks;        /**< @brief number of ticks until trigger */
    Os_TickType     cycle;        /**< @brief number of ticks in each cycle */
} Os_AlarmControlType;

/**
 * @brief Linked list of ready tasks
 */
typedef struct Os_ReadyListType {
    Os_TaskType head;             /**< @brief pointer to the first ready task */
    Os_TaskType tail;             /**< @brief pointer to the last ready task */
} Os_ReadyListType;

#define E_OS_ACCESS   (Os_StatusType)1
#define E_OS_CALLEVEL (Os_StatusType)2
#define E_OS_ID       (Os_StatusType)3
#define E_OS_LIMIT    (Os_StatusType)4
#define E_OS_NOFUNC   (Os_StatusType)5
#define E_OS_RESOURCE (Os_StatusType)6
#define E_OS_STATE    (Os_StatusType)7
#define E_OS_VALUE    (Os_StatusType)8

#define E_OS_SYS_NOT_IMPLEMENTED (Os_StatusType)16

#endif /* OS_TYPES_H_ */
