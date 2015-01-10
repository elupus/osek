#include "gtest/gtest.h"

extern "C" {
    #include "Std_Types.h"
    #include "Os.h"
    #include "Os_Cfg.h"
    #include "Os_Types.h"
}

struct Os_HooksInterface
{
    virtual void ErrorHook(Os_StatusType ret) = 0;
};

Os_HooksInterface* Os_Hooks;

extern "C" void Os_ErrorHook(Os_StatusType ret)
{
    Os_Hooks->ErrorHook(ret);
}


void Os_Test_Simple_Idle(void)
{
    Os_TerminateTask();
}


TEST(Os_Test, Simple) {
    unsigned char idle_stack[8192*16];

    Os_TaskConfigType tasks[OS_TASK_COUNT];
    tasks[0].entry      = Os_Test_Simple_Idle;
    tasks[0].stack      = idle_stack;
    tasks[0].stack_size = sizeof(idle_stack);
    tasks[0].autostart  = 1;
    tasks[0].priority   = 0;
    tasks[0].resource   = OS_INVALID_RESOURCE;

    Os_ResourceConfigType resources[OS_RES_COUNT];
    resources[0].priority = OS_PRIO_COUNT;

    Os_AlarmConfigType alarms[OS_ALARM_COUNT];
    alarms[0].task = 0;

    Os_ConfigType config;
    config.tasks     = &tasks;
    config.resources = &resources;
    config.alarms    = &alarms;

    struct Hooks : Os_HooksInterface {
        virtual      ~Hooks() {}
        virtual void ErrorHook(Os_StatusType ret)
        {
            FAIL() << "ErrorHook(" << ret << ")";
            exit(ret);
        }
    } hooks;

    Os_Hooks = &hooks;
    Os_Init(&config);
    Os_Start();
}
