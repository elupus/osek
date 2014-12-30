#include "Os.h"
#include "Os_Cfg.h"
#include "Os_Types.h"

unsigned char idle_stack[1000];

void idle(void)
{
    Os_TerminateTask();
}

unsigned char busy_stack[1000];

void busy(void)
{
    Os_TerminateTask();
}

const Os_TaskConfigType Os_DefaultTasks[OS_TASK_COUNT] = {
          { .priority  = 0,
            .entry     = idle,
            .stack_tpp = idle_stack+sizeof(idle_stack)-1,
            .autostart = 1
          }
        , { .priority  = 1,
            .entry     = busy,
            .stack_tpp = busy_stack+sizeof(busy_stack)-1,
            .autostart = 1
          }
};

const Os_ConfigType Os_DefaultConfig = {
        .tasks = &Os_DefaultTasks
};

void main()
{
    Os_Init(&Os_DefaultConfig);
    Os_Start(); /* NO RETURN */
}



