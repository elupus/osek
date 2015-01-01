#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>

#include "Os.h"
#include "Os_Cfg.h"
#include "Os_Types.h"

unsigned char idle_stack[8192*16];
unsigned char busy_stack[8192*16];
unsigned char test_stack[8192*16];

void idle(void)
{
    while(1) {
        Os_ActivateTask(1);
    }
    Os_TerminateTask();
}

void busy(void)
{
    int i;
    for(i = 0; i < 10000; i++) {
        /* NOP */
    }
    Os_ActivateTask(2);
    for(i = 0; i < 10000; i++) {
        /* NOP */
    }
    Os_TerminateTask();
}

void test(void)
{
    Os_TerminateTask();
}

const Os_TaskConfigType Os_DefaultTasks[OS_TASK_COUNT] = {
          { .priority   = 0,
            .entry      = idle,
            .stack      = idle_stack,
            .stack_size = sizeof(idle_stack),
            .autostart  = 1
          }
        , { .priority   = 1,
            .entry      = busy,
            .stack      = busy_stack,
            .stack_size = sizeof(busy_stack),
            .autostart  = 0
          }
       , { .priority    = 2,
           .entry       = test,
           .stack       = test_stack,
           .stack_size  = sizeof(test_stack),
           .autostart   = 0
         }
};

const Os_ConfigType Os_DefaultConfig = {
        .tasks = &Os_DefaultTasks
};

int main(void)
{
    Os_Init(&Os_DefaultConfig);
    Os_Start(); /* NO RETURN */
    return 0;
}



