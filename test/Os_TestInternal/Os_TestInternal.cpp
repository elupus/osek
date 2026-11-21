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
void                     (*Os_ErrorHookExtra)(void);

extern "C" void Os_ErrorHook   (Os_StatusType ret)
{
    Os_Errors.push(ret);
    if (Os_ErrorHookExtra) {
        Os_ErrorHookExtra();
    }
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

extern "C" Os_StatusType Os_Arch_Syscall(Os_SyscallParamType* param)
{
    return E_NOT_OK;
}

extern "C" void Os_Arch_Start(void)
{

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
        Os_ErrorHookExtra = NULL;
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

struct Os_TestAlarm : public Os_TestInternal
{
    virtual void SetUp()
    {
        Os_TestInternal::SetUp();
        Os_Init(&m_config);
    }
};

TEST_F(Os_TestAlarm, SetAbsAlarm1) {
    EXPECT_EQ(E_OS_ID    , Os_SetAbsAlarm_Internal(OS_ALARM_COUNT, 1, 0)) << "Alarm of invalid ID";
}

TEST_F(Os_TestAlarm, SetAbsAlarm2) {
    Os_TickType tick;
    EXPECT_EQ(E_OK       , Os_SetAbsAlarm_Internal(0, 1, 0));
    EXPECT_EQ(E_OK       , Os_SetAbsAlarm_Internal(1, 5, 0));
    EXPECT_EQ(E_OK       , Os_SetAbsAlarm_Internal(2, 3, 0));

    EXPECT_EQ(E_OK       , Os_GetAlarm_Internal(0, &tick));
    EXPECT_EQ(1          , tick);

    EXPECT_EQ(E_OK       , Os_GetAlarm_Internal(1, &tick));
    EXPECT_EQ(5          , tick);

    EXPECT_EQ(E_OK       , Os_GetAlarm_Internal(2, &tick));
    EXPECT_EQ(3          , tick);
}

TEST_F(Os_TestAlarm, SetAbsAlarm3) {
    EXPECT_EQ(E_OK       , Os_SetAbsAlarm_Internal(0, 1, 0));
    EXPECT_EQ(E_OS_STATE , Os_SetAbsAlarm_Internal(0, 1, 0)) << "Alarm should already have been set";
}

TEST_F(Os_TestAlarm, SetRelAlarm1) {
    EXPECT_EQ(E_OS_ID    , Os_SetRelAlarm_Internal(OS_ALARM_COUNT, 1, 0)) << "Alarm of invalid ID";
}

TEST_F(Os_TestAlarm, SetRelAlarm2) {
    Os_TickType tick;
    EXPECT_EQ(E_OK       , Os_SetRelAlarm_Internal(0, 1, 0));
    EXPECT_EQ(E_OK       , Os_SetRelAlarm_Internal(1, 5, 0));
    EXPECT_EQ(E_OK       , Os_SetRelAlarm_Internal(2, 3, 0));

    EXPECT_EQ(E_OK       , Os_GetAlarm_Internal(0, &tick));
    EXPECT_EQ(1          , tick);

    EXPECT_EQ(E_OK       , Os_GetAlarm_Internal(1, &tick));
    EXPECT_EQ(5          , tick);

    EXPECT_EQ(E_OK       , Os_GetAlarm_Internal(2, &tick));
    EXPECT_EQ(3          , tick);
}


TEST_F(Os_TestAlarm, SetRelAlarm3) {
    EXPECT_EQ(E_OK       , Os_SetRelAlarm_Internal(0, 1, 0));
    EXPECT_EQ(E_OS_STATE , Os_SetRelAlarm_Internal(0, 1, 0)) << "Alarm should already have been set";
}


TEST_F(Os_TestAlarm, CancelAlarm1) {
    EXPECT_EQ(E_OS_ID    , Os_CancelAlarm_Internal(OS_ALARM_COUNT)) << "Alarm of invalid ID";
}

TEST_F(Os_TestAlarm, CancelAlarm2) {
    Os_TickType tick;
    EXPECT_EQ(E_OK       , Os_SetAbsAlarm_Internal(0, 1, 0));
    EXPECT_EQ(E_OK       , Os_CancelAlarm_Internal(0));
    EXPECT_EQ(E_OS_NOFUNC, Os_CancelAlarm_Internal(0))     << "Already canceled this alarm";
}

TEST_F(Os_TestAlarm, CancelAlarm3) {
    Os_TickType tick;
    EXPECT_EQ(E_OK       , Os_SetAbsAlarm_Internal(0, 1, 0));
    EXPECT_EQ(E_OK       , Os_SetAbsAlarm_Internal(1, 5, 0));
    EXPECT_EQ(E_OK       , Os_SetAbsAlarm_Internal(2, 3, 0));

    EXPECT_EQ(E_OK       , Os_CancelAlarm_Internal(2));

    EXPECT_EQ(E_OK       , Os_GetAlarm_Internal(0, &tick));
    EXPECT_EQ(1          , tick);

    EXPECT_EQ(E_OK       , Os_GetAlarm_Internal(1, &tick));
    EXPECT_EQ(5          , tick);

    EXPECT_EQ(E_OS_NOFUNC, Os_GetAlarm_Internal(2, &tick)) << "Alarm should have been cancelled";
}

TEST_F(Os_TestAlarm, RearmAfterCancel) {
    EXPECT_EQ(E_OK       , Os_SetRelAlarm_Internal(0, 1, 0));
    EXPECT_EQ(E_OK       , Os_CancelAlarm_Internal(0));
    EXPECT_EQ(E_OK       , Os_SetRelAlarm_Internal(0, 1, 0)) << "Cancelled alarm can be armed again";
    EXPECT_EQ(E_OK       , Os_CancelAlarm_Internal(0));
    EXPECT_EQ(E_OK       , Os_SetAbsAlarm_Internal(0, 1, 0)) << "Cancelled alarm can be armed again";
}

TEST_F(Os_TestAlarm, RearmAfterExpiry) {
    Os_TickType tick;
    EXPECT_EQ(E_OK       , Os_SetRelAlarm_Internal(0, 1, 0));
    EXPECT_EQ(E_OK       , Os_IncrementCounter_Internal(OS_COUNTER_SYSTEM));
    EXPECT_EQ(E_OS_NOFUNC, Os_GetAlarm_Internal(0, &tick))   << "Alarm should have expired";
    EXPECT_EQ(E_OK       , Os_SetRelAlarm_Internal(0, 1, 0)) << "Expired alarm can be armed again";
}

TEST_F(Os_TestAlarm, AbsAlarmAtZeroInUse) {
    EXPECT_EQ(E_OK       , Os_SetAbsAlarm_Internal(0, 0, 0));
    EXPECT_EQ(E_OS_STATE , Os_SetAbsAlarm_Internal(0, 0, 0)) << "Alarm expiring at tick 0 is in use";
    EXPECT_EQ(E_OS_STATE , Os_SetRelAlarm_Internal(0, 1, 0)) << "Alarm expiring at tick 0 is in use";
}

TEST_F(Os_TestAlarm, GetAlarm1) {
    Os_TickType tick;
    EXPECT_EQ(E_OS_ID    , Os_GetAlarm_Internal(OS_ALARM_COUNT, &tick)) << "Alarm of invalid ID";
}


struct Os_TestResource : public Os_TestInternal
{
    virtual void SetUp()
    {
        Os_TestInternal::SetUp();
        Os_Init(&m_config);
    }
};

TEST_F(Os_TestResource, GetResource1) {
    EXPECT_EQ(E_OS_ID    , Os_GetResource_Internal(OS_RES_COUNT))       << "Resource of invalid ID";
}

TEST_F(Os_TestResource, ReleaseResource1) {
    EXPECT_EQ(E_OS_ID    , Os_ReleaseResource_Internal(OS_RES_COUNT))   << "Resource of invalid ID";
}

/**
 * @brief Fixture with a task running, for services called from task context
 */
struct Os_TestTask : public Os_TestInternal
{
    virtual void SetUp()
    {
        Os_TestInternal::SetUp();
        for(Os_TaskType i = 0; i < OS_TASK_COUNT; ++i) {
            m_tasks[i].activation = 1u;
        }
    }

    /* initialize and make given task the running one */
    void Start(Os_TaskType task)
    {
        Os_Init(&m_config);
        Os_CallContext = OS_CONTEXT_TASK;
        ASSERT_EQ(E_OK, Os_ActivateTask_Internal(task));
        ASSERT_EQ(task, Os_ActiveTask);
        ASSERT_EQ(OS_TASK_RUNNING, Os_TaskControls[task].state);
    }
};

TEST_F(Os_TestTask, ChainTaskLimit) {
    Start(1);
    EXPECT_EQ(E_OK           , Os_ActivateTask_Internal(0)) << "Lower priority task stays ready";
    EXPECT_EQ(E_OS_LIMIT     , Os_ChainTask_Internal(0))    << "Task already activated";
    EXPECT_EQ(1              , Os_ActiveTask)               << "Caller keeps running on error";
    EXPECT_EQ(OS_TASK_RUNNING, Os_TaskControls[1].state)    << "Caller keeps running on error";
    EXPECT_EQ(1u             , Os_TaskControls[1].activation);
}

TEST_F(Os_TestTask, ChainTaskSelf) {
    Start(1);
    EXPECT_EQ(E_OK           , Os_ChainTask_Internal(1))    << "Chaining itself is within the limit";
    EXPECT_EQ(1              , Os_ActiveTask);
    EXPECT_EQ(1u             , Os_TaskControls[1].activation);
}

TEST_F(Os_TestTask, ChainTaskOther) {
    Start(1);
    EXPECT_EQ(E_OK           , Os_ChainTask_Internal(0));
    EXPECT_EQ(0              , Os_ActiveTask);
    EXPECT_EQ(OS_TASK_SUSPENDED, Os_TaskControls[1].state);
    EXPECT_EQ(0u             , Os_TaskControls[1].activation);
}

TEST_F(Os_TestTask, ResourceNestLowerCeiling) {
    m_resources[1].priority = 3;
    m_resources[2].priority = 2;
    Start(1);
    EXPECT_EQ(E_OK       , Os_GetResource_Internal(1));
    EXPECT_EQ(3          , Os_TaskControls[1].priority);
    EXPECT_EQ(E_OK       , Os_GetResource_Internal(2))     << "Nesting a lower ceiling is allowed";
    EXPECT_EQ(3          , Os_TaskControls[1].priority)    << "Priority must not drop";
    EXPECT_EQ(E_OK       , Os_ReleaseResource_Internal(2));
    EXPECT_EQ(3          , Os_TaskControls[1].priority)    << "Back to the priority before taking it";
    EXPECT_EQ(E_OK       , Os_ReleaseResource_Internal(1));
    EXPECT_EQ(1          , Os_TaskControls[1].priority)    << "Back to the static priority";
}

TEST_F(Os_TestTask, ResourceNestHigherCeiling) {
    m_resources[1].priority = 2;
    m_resources[2].priority = 3;
    Start(1);
    EXPECT_EQ(E_OK       , Os_GetResource_Internal(1));
    EXPECT_EQ(E_OK       , Os_GetResource_Internal(2));
    EXPECT_EQ(3          , Os_TaskControls[1].priority);
    EXPECT_EQ(E_OS_NOFUNC, Os_ReleaseResource_Internal(1)) << "Release must be LIFO";
    EXPECT_EQ(E_OK       , Os_ReleaseResource_Internal(2));
    EXPECT_EQ(2          , Os_TaskControls[1].priority);
    EXPECT_EQ(E_OK       , Os_ReleaseResource_Internal(1));
    EXPECT_EQ(1          , Os_TaskControls[1].priority);
}

TEST_F(Os_TestTask, ResourceCeilingBelowTask) {
    m_resources[1].priority = 0;
    Start(1);
    EXPECT_EQ(E_OS_ACCESS, Os_GetResource_Internal(1))     << "Ceiling below the task's priority";
}

TEST_F(Os_TestTask, InternalResourceOnActivate) {
    m_resources[1].priority = 3;
    m_tasks[2].resource     = 1;
    Start(2);
    EXPECT_EQ(1          , Os_TaskControls[2].resource) << "Internal resource held when started by activation";
    EXPECT_EQ(3          , Os_TaskControls[2].priority);
}

TEST_F(Os_TestTask, InternalResourceOnAlarm) {
    m_resources[1].priority = 3;
    m_tasks[2].resource     = 1;
    m_alarms[0].task        = 2;
    Start(0);
    EXPECT_EQ(E_OK       , Os_SetRelAlarm_Internal(0, 1, 0));
    Os_Isr();
    EXPECT_EQ(2          , Os_ActiveTask)               << "Alarm activated task preempts";
    EXPECT_EQ(1          , Os_TaskControls[2].resource) << "Internal resource held when started from the tick";
    EXPECT_EQ(3          , Os_TaskControls[2].priority);
}

TEST_F(Os_TestTask, InternalResourceNonPreemptive) {
    m_resources[1].priority = 3;
    m_tasks[1].resource     = 1;
    Start(1);
    EXPECT_EQ(E_OK       , Os_ActivateTask_Internal(2));
    EXPECT_EQ(1          , Os_ActiveTask)               << "Higher priority task below the ceiling does not preempt";

    Os_SyscallParamType param;
    param.service = OSServiceId_Schedule;
    EXPECT_EQ(E_OK       , Os_Syscall_Internal(&param));
    EXPECT_EQ(2          , Os_ActiveTask)               << "Schedule lets it run";
    EXPECT_EQ(OS_INVALID_RESOURCE, Os_TaskControls[1].resource) << "Internal resource released while preempted";

    param.service = OSServiceId_TerminateTask;
    EXPECT_EQ(E_OK       , Os_Syscall_Internal(&param));
    EXPECT_EQ(1          , Os_ActiveTask);
    EXPECT_EQ(1          , Os_TaskControls[1].resource) << "Internal resource taken again on resume";
    EXPECT_EQ(3          , Os_TaskControls[1].priority);
}

TEST_F(Os_TestTask, GetTaskIdBeforeStart) {
    Os_TaskType task = 0;
    Os_Init(&m_config);
    EXPECT_EQ(OS_INVALID_TASK, Os_ActiveTask);
    EXPECT_EQ(E_OK           , Os_GetTaskId(&task));
    EXPECT_EQ(OS_INVALID_TASK, task) << "No task has run yet";
}

TEST_F(Os_TestTask, ScheduleFromInit) {
    Os_TaskType task;
    m_tasks[2].autostart = TRUE;
    Os_Init(&m_config);
    Os_CallContext = OS_CONTEXT_TASK;
    EXPECT_EQ(E_OK           , Os_Schedule_Internal());
    EXPECT_EQ(E_OK           , Os_GetTaskId(&task));
    EXPECT_EQ(2              , task) << "Autostart task runs first";
}

TEST_F(Os_TestTask, GetTaskIdAfterTerminate) {
    Os_TaskType task;
    Start(1);
    EXPECT_EQ(E_OK           , Os_TerminateTask_Internal());
    EXPECT_EQ(E_OK           , Os_GetTaskId(&task));
    EXPECT_EQ(OS_INVALID_TASK, task) << "Nothing running after the last task terminated";
}

TEST_F(Os_TestTask, InitInternalResourceCeiling) {
    m_resources[1].priority = 0;
    m_tasks[1].resource     = 1;
    m_tasks[2].autostart    = TRUE;
    Os_Init(&m_config);
    ASSERT_FALSE(Os_Errors.empty());
    EXPECT_EQ(E_OS_RESOURCE, Os_Errors.top()) << "Ceiling below the task's priority";
    EXPECT_FALSE(Os_Initialized);
    Os_ActiveTask = OS_INVALID_TASK; /* Os_Init left it untouched, clear what earlier tests left */
    Os_Start();
    EXPECT_EQ(OS_INVALID_TASK, Os_ActiveTask) << "Os_Start does not start on a bad configuration";
}

TEST_F(Os_TestTask, InitInternalResourceId) {
    m_tasks[1].resource     = OS_RES_COUNT;
    Os_Init(&m_config);
    ASSERT_FALSE(Os_Errors.empty());
    EXPECT_EQ(E_OS_ID      , Os_Errors.top()) << "Internal resource does not exist";
    EXPECT_FALSE(Os_Initialized);
}

TEST_F(Os_TestTask, InitTaskPriority) {
    m_tasks[1].priority     = OS_PRIO_COUNT;
    Os_Init(&m_config);
    ASSERT_FALSE(Os_Errors.empty());
    EXPECT_EQ(E_OS_VALUE   , Os_Errors.top()) << "Priority outside the configured levels";
    EXPECT_FALSE(Os_Initialized);
}

TEST_F(Os_TestTask, InitValid) {
    m_resources[1].priority = 3;
    m_tasks[1].resource     = 1;
    Os_Init(&m_config);
    EXPECT_TRUE(Os_Errors.empty());
    EXPECT_TRUE(Os_Initialized);
}

/* advance the system counter */
static void Os_TestTicks(uint32_t ticks)
{
    for (uint32_t i = 0; i < ticks; ++i) {
        Os_IncrementCounter_Internal(OS_COUNTER_SYSTEM);
    }
}

/* advance the system counter, returns false if alarm expired before all ticks */
static bool Os_TestAdvance(Os_AlarmType alarm, uint32_t ticks)
{
    Os_TickType tick;
    for (uint32_t i = 0; i < ticks; ++i) {
        if (Os_GetAlarm_Internal(alarm, &tick) != E_OK) {
            return false;
        }
        Os_IncrementCounter_Internal(OS_COUNTER_SYSTEM);
    }
    return true;
}

TEST_F(Os_TestAlarm, RelAlarmLargeIncrement) {
    Os_TickType tick;
    EXPECT_EQ(E_OK       , Os_SetRelAlarm_Internal(0, 40000u, 0));
    EXPECT_TRUE(Os_TestAdvance(0, 39999u))                    << "Expired early";
    EXPECT_EQ(E_OK       , Os_GetAlarm_Internal(0, &tick));
    EXPECT_EQ(1u         , tick);
    EXPECT_EQ(E_OK       , Os_IncrementCounter_Internal(OS_COUNTER_SYSTEM));
    EXPECT_EQ(E_OS_NOFUNC, Os_GetAlarm_Internal(0, &tick))    << "Expired after the increment";
}

TEST_F(Os_TestAlarm, RelAlarmMaxIncrement) {
    Os_TickType tick;
    EXPECT_EQ(E_OK       , Os_SetRelAlarm_Internal(0, OS_MAXALLOWEDVALUE, 0));
    EXPECT_TRUE(Os_TestAdvance(0, OS_MAXALLOWEDVALUE - 1u))   << "Expired early";
    EXPECT_EQ(E_OK       , Os_IncrementCounter_Internal(OS_COUNTER_SYSTEM));
    EXPECT_EQ(E_OS_NOFUNC, Os_GetAlarm_Internal(0, &tick));
}

TEST_F(Os_TestAlarm, AbsAlarmInPast) {
    Os_TickType tick;
    Os_TestTicks(10u);
    EXPECT_EQ(E_OK       , Os_SetAbsAlarm_Internal(0, 5u, 0));
    EXPECT_TRUE(Os_TestAdvance(0, 65535u - 10u + 5u))         << "Start already passed, waits for the counter to wrap";
    EXPECT_EQ(E_OK       , Os_IncrementCounter_Internal(OS_COUNTER_SYSTEM));
    EXPECT_EQ(5u         , Os_CounterControls[OS_COUNTER_SYSTEM].ticks);
    EXPECT_EQ(E_OS_NOFUNC, Os_GetAlarm_Internal(0, &tick))    << "Expired when the counter reached start";
}

TEST_F(Os_TestAlarm, AbsAlarmNow) {
    Os_TickType tick;
    Os_TestTicks(10u);
    EXPECT_EQ(E_OK       , Os_SetAbsAlarm_Internal(0, 10u, 0));
    EXPECT_TRUE(Os_TestAdvance(0, 65535u))                    << "Start reached, waits a full counter period";
    EXPECT_EQ(E_OK       , Os_IncrementCounter_Internal(OS_COUNTER_SYSTEM));
    EXPECT_EQ(E_OS_NOFUNC, Os_GetAlarm_Internal(0, &tick));
}

TEST_F(Os_TestAlarm, CyclicEveryTick) {
    Os_TickType tick;
    EXPECT_EQ(E_OK       , Os_SetRelAlarm_Internal(0, 1u, 1u));
    for (int i = 0; i < 3; ++i) {
        EXPECT_EQ(E_OK   , Os_IncrementCounter_Internal(OS_COUNTER_SYSTEM));
        EXPECT_EQ(E_OK   , Os_GetAlarm_Internal(0, &tick))    << "Cyclic alarm re-armed";
        EXPECT_EQ(1u     , tick)                              << "Cyclic alarm expires once per tick";
    }
}

TEST_F(Os_TestAlarm, OrderAcrossWrap) {
    Os_TickType tick;
    Os_TestTicks(65530u);
    EXPECT_EQ(E_OK       , Os_SetRelAlarm_Internal(0, 10u, 0)); /* expires at 4, after the wrap */
    EXPECT_EQ(E_OK       , Os_SetRelAlarm_Internal(1,  3u, 0)); /* expires at 65533 */
    EXPECT_EQ(E_OK       , Os_SetAbsAlarm_Internal(2,  2u, 0)); /* expires at 2, after the wrap */
    EXPECT_TRUE(Os_TestAdvance(1, 2u));
    EXPECT_EQ(E_OK       , Os_IncrementCounter_Internal(OS_COUNTER_SYSTEM));
    EXPECT_EQ(E_OS_NOFUNC, Os_GetAlarm_Internal(1, &tick))    << "Alarm before the wrap expires first";
    EXPECT_EQ(E_OK       , Os_GetAlarm_Internal(2, &tick));
    EXPECT_EQ(5u         , tick);
    EXPECT_EQ(E_OK       , Os_GetAlarm_Internal(0, &tick));
    EXPECT_EQ(7u         , tick);
}

TEST_F(Os_TestAlarm, OrderAfterPop) {
    Os_TickType tick;
    /* queue becomes [1, 5, 2, 6], popping 1 moves 6 to the top */
    EXPECT_EQ(E_OK       , Os_SetRelAlarm_Internal(0, 1u, 0));
    EXPECT_EQ(E_OK       , Os_SetRelAlarm_Internal(1, 5u, 0));
    EXPECT_EQ(E_OK       , Os_SetRelAlarm_Internal(2, 2u, 0));
    EXPECT_EQ(E_OK       , Os_SetRelAlarm_Internal(3, 6u, 0));
    Os_TestTicks(2u);
    EXPECT_EQ(E_OS_NOFUNC, Os_GetAlarm_Internal(2, &tick))    << "Alarm at 2 expired at 2";
    EXPECT_EQ(E_OK       , Os_GetAlarm_Internal(1, &tick));
    Os_TestTicks(3u);
    EXPECT_EQ(E_OS_NOFUNC, Os_GetAlarm_Internal(1, &tick))    << "Alarm at 5 expired at 5";
    EXPECT_EQ(E_OK       , Os_GetAlarm_Internal(3, &tick));
    Os_TestTicks(1u);
    EXPECT_EQ(E_OS_NOFUNC, Os_GetAlarm_Internal(3, &tick))    << "Alarm at 6 expired at 6";
}

TEST_F(Os_TestAlarm, OrderAfterCancel) {
    Os_TickType tick;
    /* queue becomes [1, 10, 2, 11, 12, 3], cancelling 11 moves 3 below 10 */
    EXPECT_EQ(E_OK       , Os_SetRelAlarm_Internal(0,  1u, 0));
    EXPECT_EQ(E_OK       , Os_SetRelAlarm_Internal(1, 10u, 0));
    EXPECT_EQ(E_OK       , Os_SetRelAlarm_Internal(2,  2u, 0));
    EXPECT_EQ(E_OK       , Os_SetRelAlarm_Internal(3, 11u, 0));
    EXPECT_EQ(E_OK       , Os_SetRelAlarm_Internal(4, 12u, 0));
    EXPECT_EQ(E_OK       , Os_SetRelAlarm_Internal(5,  3u, 0));
    EXPECT_EQ(E_OK       , Os_CancelAlarm_Internal(3));
    const Os_TickType expiry[] = { 1u, 10u, 2u, 0u, 12u, 3u };
    for (Os_TickType now = 1u; now <= 12u; ++now) {
        Os_TestTicks(1u);
        for (Os_AlarmType a = 0u; a < 6u; ++a) {
            if (a == 3u) {
                continue;
            }
            EXPECT_EQ(expiry[a] <= now ? E_OS_NOFUNC : E_OK, Os_GetAlarm_Internal(a, &tick))
                << "Alarm " << (int)a << " at tick " << (int)now;
        }
    }
}

TEST_F(Os_TestAlarm, RandomAgainstModel) {
    /* every alarm must expire exactly at the tick a simple model predicts */
    uint32_t    seed = 12345u;
    auto        rnd  = [&seed](uint32_t n) { seed = seed * 1103515245u + 12345u; return (seed >> 8) % n; };
    bool        armed [OS_ALARM_COUNT] = {};
    Os_TickType expiry[OS_ALARM_COUNT] = {};
    Os_TickType cycle [OS_ALARM_COUNT] = {};
    Os_TickType now = 0u, tick;

    for (int step = 0; step < 20000; ++step) {
        Os_AlarmType a = (Os_AlarmType)rnd(OS_ALARM_COUNT);
        switch (rnd(4)) {
        case 0:
            if (!armed[a]) {
                Os_TickType inc = (Os_TickType)(1u + rnd(rnd(2) ? 20u : 65535u));
                cycle[a]  = (Os_TickType)(rnd(3) ? 0u : 1u + rnd(15u));
                ASSERT_EQ(E_OK, Os_SetRelAlarm_Internal(a, inc, cycle[a]));
                expiry[a] = (Os_TickType)(now + inc);
                armed[a]  = true;
            }
            break;
        case 1:
            if (!armed[a]) {
                Os_TickType start = (Os_TickType)rnd(65536u);
                cycle[a]  = 0u;
                ASSERT_EQ(E_OK, Os_SetAbsAlarm_Internal(a, start, 0u));
                expiry[a] = start;
                armed[a]  = true;
            }
            break;
        case 2:
            if (armed[a] && rnd(4) == 0u) {
                ASSERT_EQ(E_OK, Os_CancelAlarm_Internal(a));
                armed[a] = false;
            }
            break;
        default:
            ASSERT_EQ(E_OK, Os_IncrementCounter_Internal(OS_COUNTER_SYSTEM));
            now++;
            for (Os_AlarmType i = 0u; i < OS_ALARM_COUNT; ++i) {
                if (armed[i] && expiry[i] == now) {
                    if (cycle[i]) {
                        expiry[i] = (Os_TickType)(expiry[i] + cycle[i]);
                    } else {
                        armed[i] = false;
                    }
                }
            }
            break;
        }
        for (Os_AlarmType i = 0u; i < OS_ALARM_COUNT; ++i) {
            ASSERT_EQ(armed[i] ? E_OK : E_OS_NOFUNC, Os_GetAlarm_Internal(i, &tick))
                << "alarm " << (int)i << " step " << step;
            if (armed[i]) {
                ASSERT_EQ((Os_TickType)(expiry[i] - now), tick) << "alarm " << (int)i << " step " << step;
            }
        }
    }
}

TEST_F(Os_TestTask, GetTaskState) {
    Os_TaskStateType state;
    Start(1);
    EXPECT_EQ(E_OK             , Os_ActivateTask_Internal(0));
    EXPECT_EQ(E_OK             , Os_GetTaskState_Internal(1, &state));
    EXPECT_EQ(OS_TASK_RUNNING  , state);
    EXPECT_EQ(E_OK             , Os_GetTaskState_Internal(0, &state));
    EXPECT_EQ(OS_TASK_READY    , state) << "Activated but not started is ready";
    EXPECT_EQ(E_OK             , Os_GetTaskState_Internal(2, &state));
    EXPECT_EQ(OS_TASK_SUSPENDED, state);
    EXPECT_EQ(E_OK             , Os_ActivateTask_Internal(2));
    EXPECT_EQ(E_OK             , Os_GetTaskState_Internal(1, &state));
    EXPECT_EQ(OS_TASK_READY    , state) << "Preempted is ready";
    EXPECT_EQ(E_OS_ID          , Os_GetTaskState_Internal(OS_TASK_COUNT, &state));
}

TEST_F(Os_TestTask, ErrorHookNotRecursive) {
    Start(1);
    Os_ErrorHookExtra = []() {
        EXPECT_EQ(E_OS_ID, Os_ActivateTask_Internal(OS_TASK_COUNT)) << "Service fails inside the hook";
    };
    EXPECT_EQ(E_OS_ID  , Os_ActivateTask_Internal(OS_TASK_COUNT));
    EXPECT_EQ(1u       , Os_Errors.size()) << "Hook not called again from inside the hook";
    EXPECT_FALSE(Os_ErrorHookActive);
}

TEST_F(Os_TestTask, InternalResourceKeptOnError) {
    Os_SyscallParamType param;
    m_resources[1].priority = 3;
    m_tasks[1].resource     = 1;
    Start(1);
    EXPECT_EQ(E_OK       , Os_ActivateTask_Internal(0));
    param.service = OSServiceId_ChainTask;
    param.p1.task = 0;
    EXPECT_EQ(E_OS_LIMIT , Os_Syscall_Internal(&param));
    EXPECT_EQ(1          , Os_ActiveTask);
    EXPECT_EQ(1          , Os_TaskControls[1].resource) << "Internal resource held after a failed ChainTask";
    EXPECT_EQ(3          , Os_TaskControls[1].priority);
}

TEST_F(Os_TestTask, InternalResourceNotTakenWhenIdle) {
    Os_SyscallParamType param;
    m_resources[1].priority = 3;
    m_tasks[1].resource     = 1;
    Start(1);
    param.service = OSServiceId_TerminateTask;
    EXPECT_EQ(E_OK       , Os_Syscall_Internal(&param));
    EXPECT_EQ(OS_INVALID_RESOURCE, Os_TaskControls[1].resource) << "Terminated task does not hold it";
    EXPECT_EQ(OS_INVALID_TASK    , Os_ResourceControls[1].task);
}

TEST_F(Os_TestTask, InternalResourceTerminate) {
    Os_SyscallParamType param;
    m_resources[1].priority = 3;
    m_tasks[1].resource     = 1;
    m_tasks[2].resource     = 1;
    Start(1);
    EXPECT_EQ(E_OK       , Os_ActivateTask_Internal(2));
    EXPECT_EQ(1          , Os_ActiveTask) << "Same group, does not preempt";
    param.service = OSServiceId_TerminateTask;
    EXPECT_EQ(E_OK       , Os_Syscall_Internal(&param)) << "Terminate with only the internal resource held";
    EXPECT_EQ(2          , Os_ActiveTask);
    EXPECT_EQ(1          , Os_TaskControls[2].resource) << "Next task in the group takes it";
    EXPECT_EQ(2          , Os_ResourceControls[1].task);
}

TEST_F(Os_TestTask, InternalResourceScheduleNoSwitch) {
    Os_SyscallParamType param;
    m_resources[1].priority = 3;
    m_tasks[1].resource     = 1;
    Start(1);
    param.service = OSServiceId_Schedule;
    EXPECT_EQ(E_OK       , Os_Syscall_Internal(&param));
    EXPECT_EQ(1          , Os_ActiveTask);
    EXPECT_EQ(1          , Os_TaskControls[1].resource) << "Kept when nothing else runs";
    EXPECT_EQ(3          , Os_TaskControls[1].priority);
}
