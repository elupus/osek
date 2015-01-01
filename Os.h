#ifndef OS_H_
#define OS_H_

#include "Os_Types.h"
#include "Os_Cfg.h"

typedef struct Os_ConfigType {
    const Os_TaskConfigType (*tasks)[OS_TASK_COUNT];
} Os_ConfigType;


extern Os_TaskControlType              Os_TaskControls        [OS_TASK_COUNT];
extern Os_ReadyListType                Os_TaskReady           [OS_PRIO_COUNT];
extern Os_TaskType                     Os_TaskRunning;
extern Os_ContextType                  Os_CallContext;
extern const Os_TaskConfigType *       Os_TaskConfigs;

void       Os_Init(const Os_ConfigType* config);
void       Os_Isr(void);
StatusType Os_Schedule(void);
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
