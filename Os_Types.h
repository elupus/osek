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

#ifndef OS_TYPES_H_
#define OS_TYPES_H_

#include "Std_Types.h"

typedef uint8 Os_TaskType;        /**< task identifier */
typedef uint8 Os_ResourceType;    /**< resource identifier */
typedef uint8 Os_PriorityType;    /**< priority level */

typedef uint8 StatusType;         /**< return value for os functions */

static const Os_TaskType     Os_TaskIdNone      = (Os_TaskType)(-1);
static const Os_ResourceType Os_ResourceIdNone  = (Os_ResourceType)(-1);

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
    Os_PriorityType  priority;    /**< fixed priority of task */
    Os_TaskEntryType entry;       /**< entry point of task */
    void*            stack;       /**< bottom of stack pointer */
    size_t           stack_size;  /**< how large is the stack pointed to by stack */
    int              autostart;   /**< should this task start automatically */
    Os_ResourceType  resource;    /**< internal resource of task, can be Os_TaskIdNone */
} Os_TaskConfigType;

/**
 * @brief Structure holding information of the state of a task
 */
typedef struct Os_TaskControlType {
    Os_TaskStateEnum state;       /**< current state */
    uint8            activation;  /**< number of activations for given task */
    Os_TaskType      next;        /**< next task in the same ready list */
    Os_ResourceType  resource;    /**< last taken resource for task (rest is linked list */
} Os_TaskControlType;

typedef struct Os_ResourceConfigType {
    Os_PriorityType priority;     /**< priority ceiling of all task using */
} Os_ResourceConfigType;

typedef struct Os_ResourceControlType {
    Os_ResourceType next;         /**< linked list of held resources */
    Os_TaskType     task;         /**< task currently holding resource */
} Os_ResourceControlType;

typedef struct Os_ReadyListType {
    Os_TaskType head;             /**< pointer to the first ready task */
    Os_TaskType tail;             /**< pointer to the last ready task */
} Os_ReadyListType;

#define E_OS_ACCESS   (StatusType)1
#define E_OS_CALLEVEL (StatusType)2
#define E_OS_ID       (StatusType)3
#define E_OS_LIMIT    (StatusType)4
#define E_OS_NOFUNC   (StatusType)5
#define E_OS_RESOURCE (StatusType)6
#define E_OS_STATE    (StatusType)7
#define E_OS_VALUE    (StatusType)8

#define E_OS_SYS_NOT_IMPLEMENTED (StatusType)16

#endif /* OS_TYPES_H_ */
