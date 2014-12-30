#ifndef OS_TYPES_H_
#define OS_TYPES_H_

#include <stdint.h>

typedef uint8_t Os_TaskType;
typedef uint8_t Os_PriorityType;

typedef uint8_t StatusType;

static const Os_TaskType Os_TaskIdNone = (Os_TaskType)(-1);

typedef struct Os_TaskConfigType {
    Os_PriorityType priority;
    void          (*entry)(void);
    void*           stack_top;
    int             autostart;
} Os_TaskConfigType;

typedef struct Os_TaskControlType {
    uint8_t     active;
    Os_TaskType next;
} Os_TaskControlType;

typedef struct Os_ReadyListType {
    Os_TaskType head;
    Os_TaskType tail;
} Os_ReadyListType;

#endif /* OS_TYPES_H_ */
