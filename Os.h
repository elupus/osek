#ifndef OS_H_
#define OS_H_

#include "Os_Types.h"
#include "Os_Cfg.h"

typedef struct Os_ConfigType {
    const Os_TaskConfigType (*tasks)[OS_TASK_COUNT];
} Os_ConfigType;

void       Os_Start(void);
StatusType Os_TerminateTask(void);
StatusType Os_ActivateTask(Os_TaskType task);

#ifndef E_OK
#define E_OK 0u
#endif

#ifndef E_NOT_OK
#define E_NOT_OK 1u
#endif

#ifndef E_OS_LIMIT
#define E_OS_LIMIT 2u
#endif

#endif /* OS_H_ */
