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

typedef uint8 Os_TaskType;
typedef uint8 Os_PriorityType;

typedef uint8 StatusType;

static const Os_TaskType Os_TaskIdNone = (Os_TaskType)(-1);

typedef enum Os_ContextType {
    OS_CONTEXT_TASK = 0,
    OS_CONTEXT_ISR  = 1,
} Os_ContextType;

typedef enum Os_TaskStateEnum {
    OS_TASK_SUSPENDED = 0,
    OS_TASK_READY     = 1,
    OS_TASK_WAITING   = 2,
    OS_TASK_RUNNING   = 3,
} Os_TaskStateEnum;

typedef void          (*Os_TaskEntryType)(void);

typedef struct Os_TaskConfigType {
    Os_PriorityType  priority;
    Os_TaskEntryType entry;
    void*            stack;
    size_t           stack_size;
    int              autostart;
} Os_TaskConfigType;

typedef struct Os_TaskControlType {
    Os_TaskStateEnum state;
    uint8            activation;
    Os_TaskType      next;
} Os_TaskControlType;

typedef struct Os_ReadyListType {
    Os_TaskType head;
    Os_TaskType tail;
} Os_ReadyListType;

#define E_OS_ACCESS   (StatusType)1
#define E_OS_CALLEVEL (StatusType)2
#define E_OS_ID       (StatusType)3
#define E_OS_LIMIT    (StatusType)4
#define E_OS_NOFUNC   (StatusType)5
#define E_OS_RESOURCE (StatusType)6
#define E_OS_STATE    (StatusType)7
#define E_OS_VALUE    (StatusType)8

#endif /* OS_TYPES_H_ */
