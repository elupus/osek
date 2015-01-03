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

#include <stdint.h>
#include <stddef.h>

typedef uint8_t Os_TaskType;
typedef uint8_t Os_PriorityType;

typedef uint8_t StatusType;

static const Os_TaskType Os_TaskIdNone = (Os_TaskType)(-1);

typedef enum Os_ContextType {
    OS_CONTEXT_TASK = 0,
    OS_CONTEXT_ISR  = 1,
} Os_ContextType;

typedef void          (*Os_TaskEntryType)(void);

typedef struct Os_TaskConfigType {
    Os_PriorityType  priority;
    Os_TaskEntryType entry;
    void*            stack;
    size_t           stack_size;
    int              autostart;
} Os_TaskConfigType;

typedef struct Os_TaskControlType {
    uint8_t          activation;
    Os_TaskType      next;
} Os_TaskControlType;

typedef struct Os_ReadyListType {
    Os_TaskType head;
    Os_TaskType tail;
} Os_ReadyListType;

#endif /* OS_TYPES_H_ */
