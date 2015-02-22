#include "gtest/gtest.h"
#include <stack>
#include <map>

extern "C" {
    #include "Std_Types.h"
    #include "Os.h"
    #include "Os_Cfg.h"
    #include "Os_Types.h"

    #include "Os.c"
}

std::stack<Os_StatusType>  Os_Errors;

extern "C" void Os_ErrorHook   (Os_StatusType ret)
{
    Os_Errors.push(ret);
}

extern "C" void Os_PreTaskHook (Os_TaskType task)
{
}

extern "C" void Os_PostTaskHook(Os_TaskType task)
{
}



extern "C" void Os_Arch_Init(void)
{
}

extern "C" void Os_Arch_SuspendInterrupts(Os_IrqState* mask)
{
}

extern "C" void Os_Arch_ResumeInterrupts(const Os_IrqState* mask)
{
}

extern "C" void Os_Arch_DisableAllInterrupts(void)
{
}

extern "C" void Os_Arch_EnableAllInterrupts(void)
{
}

extern "C" void Os_Arch_PrepareState(Os_TaskType task)
{
}

extern "C" void Os_Arch_SwapState(Os_TaskType task, Os_TaskType prev)
{
    throw std::exception();
}


struct Os_TestInternal : public testing::Test {
    static Os_TestInternal* active;

    virtual void SetUp()
    {
        memset(m_tasks    , 0, sizeof(m_tasks));
        memset(m_resources, 0, sizeof(m_resources));
        memset(m_alarms   , 0, sizeof(m_alarms));
        for(Os_TaskType i = 0; i < OS_TASK_COUNT; ++i) {
            m_tasks[i].priority = (Os_PriorityType)i;
            m_tasks[i].resource = OS_INVALID_RESOURCE;
            m_tasks[i].stack    = m_stacks[i];
            m_tasks[i].stack_size = 256;
        }
        for(Os_ResourceType i = 0; i < OS_RES_COUNT; ++i) {
            ;
        }
        for(Os_AlarmType i = 0; i < OS_ALARM_COUNT; ++i) {
            m_alarms[i].task    = OS_INVALID_TASK;
            m_alarms[i].counter = OS_COUNTER_SYSTEM;
        }

        m_resources[OS_RES_SCHEDULER].priority = OS_PRIO_COUNT;

        m_config.tasks     = &m_tasks;
        m_config.resources = &m_resources;
        m_config.alarms    = &m_alarms;
        active             = this;
    }

    virtual void TearDown()
    {
        Os_Errors = std::stack<Os_StatusType>();
        for(Os_TaskType i = 0; i < OS_TASK_COUNT; ++i) {
            ;
        }
    }

    uint8_t               (m_stacks[256])[OS_TASK_COUNT];
    Os_TaskConfigType     m_tasks    [OS_TASK_COUNT];
    Os_ResourceConfigType m_resources[OS_RES_COUNT];
    Os_AlarmConfigType    m_alarms   [OS_ALARM_COUNT];
    Os_ConfigType         m_config;
};

Os_TestInternal* Os_TestInternal::active = NULL;

TEST_F(Os_TestInternal, Main) {
    Os_Init(&m_config);
    Os_TickType tick;
    EXPECT_EQ(E_OK       , Os_SetAbsAlarm_Internal(0, 1, 0));
    EXPECT_EQ(E_OK       , Os_SetAbsAlarm_Internal(1, 5, 0));
    EXPECT_EQ(E_OK       , Os_SetAbsAlarm_Internal(2, 3, 0));

    EXPECT_EQ(E_OK       , Os_GetAlarm(0, &tick));
    EXPECT_EQ(1          , tick);

    EXPECT_EQ(E_OK       , Os_GetAlarm(1, &tick));
    EXPECT_EQ(5          , tick);

    EXPECT_EQ(E_OK       , Os_GetAlarm(2, &tick));
    EXPECT_EQ(3          , tick);
}
