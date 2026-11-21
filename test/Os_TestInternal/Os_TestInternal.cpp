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
        m_config.counters  = NULL;
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
    m_tasks[2].autostart = OS_APPMODE_MASK(OS_DEFAULTAPPMODE);
    Os_Init(&m_config);
    Os_Autostart(OS_DEFAULTAPPMODE);
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
    Os_Start(OS_DEFAULTAPPMODE);
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

/* every alarm must expire exactly at the tick a simple model predicts,
 * on a counter wrapping after max */
static void Os_TestRandomAlarms(Os_TickType max, Os_TickType mincycle)
{
    uint32_t    range = (uint32_t)max + 1u;
    uint32_t    seed  = 12345u;
    auto        rnd   = [&seed](uint32_t n) { seed = seed * 1103515245u + 12345u; return (seed >> 8) % n; };
    bool        armed [OS_ALARM_COUNT] = {};
    uint32_t    expiry[OS_ALARM_COUNT] = {};
    uint32_t    cycle [OS_ALARM_COUNT] = {};
    uint32_t    now = 0u;
    Os_TickType tick;

    for (int step = 0; step < 20000; ++step) {
        Os_AlarmType a = (Os_AlarmType)rnd(OS_ALARM_COUNT);
        switch (rnd(4)) {
        case 0:
            if (!armed[a]) {
                uint32_t inc = 1u + rnd(rnd(2) ? (max < 20u ? max : 20u) : max);
                cycle[a]  = rnd(3) ? 0u : mincycle + rnd(15u);
                ASSERT_EQ(E_OK, Os_SetRelAlarm_Internal(a, (Os_TickType)inc, (Os_TickType)cycle[a]));
                expiry[a] = (now + inc) % range;
                armed[a]  = true;
            }
            break;
        case 1:
            if (!armed[a]) {
                uint32_t start = rnd(range);
                cycle[a]  = 0u;
                ASSERT_EQ(E_OK, Os_SetAbsAlarm_Internal(a, (Os_TickType)start, 0u));
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
            now = (now + 1u) % range;
            ASSERT_EQ(now, Os_CounterControls[OS_COUNTER_SYSTEM].ticks);
            for (Os_AlarmType i = 0u; i < OS_ALARM_COUNT; ++i) {
                if (armed[i] && expiry[i] == now) {
                    if (cycle[i]) {
                        expiry[i] = (expiry[i] + cycle[i]) % range;
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
                ASSERT_EQ((expiry[i] + range - now) % range, tick) << "alarm " << (int)i << " step " << step;
            }
        }
    }
}

TEST_F(Os_TestAlarm, RandomAgainstModel) {
    Os_TestRandomAlarms(OS_MAXALLOWEDVALUE, 1u);
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

static int Os_TestCallbackCount;
static void Os_TestCallback(void)
{
    Os_TestCallbackCount++;
}

TEST_F(Os_TestAlarm, Callback) {
    m_alarms[1].callback = Os_TestCallback;
    Os_TestCallbackCount = 0;
    EXPECT_EQ(E_OK       , Os_SetRelAlarm_Internal(1, 2u, 3u));
    Os_TestTicks(1u);
    EXPECT_EQ(0          , Os_TestCallbackCount);
    Os_TestTicks(1u);
    EXPECT_EQ(1          , Os_TestCallbackCount) << "Callback on expiry";
    Os_TestTicks(3u);
    EXPECT_EQ(2          , Os_TestCallbackCount) << "Callback on every cycle";
}

struct Os_TestCounter : public Os_TestInternal
{
    Os_CounterConfigType m_counters[OS_COUNTER_COUNT];

    void Init(Os_TickType max, Os_TickType ticksperbase, Os_TickType mincycle)
    {
        for (Os_CounterType i = 0u; i < OS_COUNTER_COUNT; ++i) {
            m_counters[i].maxallowedvalue = max;
            m_counters[i].ticksperbase    = ticksperbase;
            m_counters[i].mincycle        = mincycle;
        }
        m_config.counters = &m_counters;
        Os_Init(&m_config);
    }
};

TEST_F(Os_TestCounter, Defaults) {
    Os_AlarmBaseType info;
    Os_Init(&m_config);
    EXPECT_EQ(E_OK              , Os_GetAlarmBase_Internal(0, &info));
    EXPECT_EQ(OS_MAXALLOWEDVALUE, info.maxallowedvalue);
    EXPECT_EQ(OS_TICKSPERBASE   , info.ticksperbase);
    EXPECT_EQ(OS_MINCYCLE       , info.mincycle);
    EXPECT_EQ(E_OS_ID           , Os_GetAlarmBase_Internal(OS_ALARM_COUNT, &info));
}

TEST_F(Os_TestCounter, Configured) {
    Os_AlarmBaseType info;
    Init(99u, 10u, 5u);
    EXPECT_TRUE(Os_Initialized);
    EXPECT_EQ(E_OK              , Os_GetAlarmBase_Internal(0, &info));
    EXPECT_EQ(99u               , info.maxallowedvalue);
    EXPECT_EQ(10u               , info.ticksperbase);
    EXPECT_EQ(5u                , info.mincycle);
}

TEST_F(Os_TestCounter, Limits) {
    Init(99u, 1u, 5u);
    EXPECT_EQ(E_OS_VALUE        , Os_SetRelAlarm_Internal(0, 100u, 0u)) << "Increment above maxallowedvalue";
    EXPECT_EQ(E_OS_VALUE        , Os_SetRelAlarm_Internal(0, 10u, 4u))  << "Cycle below mincycle";
    EXPECT_EQ(E_OS_VALUE        , Os_SetRelAlarm_Internal(0, 10u, 100u))<< "Cycle above maxallowedvalue";
    EXPECT_EQ(E_OS_VALUE        , Os_SetAbsAlarm_Internal(0, 100u, 0u)) << "Start above maxallowedvalue";
    EXPECT_EQ(E_OS_VALUE        , Os_SetAbsAlarm_Internal(0, 10u, 4u))  << "Cycle below mincycle";
    EXPECT_EQ(E_OK              , Os_SetRelAlarm_Internal(0, 99u, 5u));
    EXPECT_EQ(E_OK              , Os_SetAbsAlarm_Internal(1, 99u, 99u));
}

TEST_F(Os_TestCounter, Wrap) {
    Os_TickType tick;
    Init(99u, 1u, 1u);
    Os_TestTicks(95u);
    EXPECT_EQ(E_OK              , Os_SetRelAlarm_Internal(0, 10u, 0u));
    EXPECT_EQ(E_OK              , Os_SetAbsAlarm_Internal(1, 50u, 0u)); /* reached after the wrap */
    Os_TestTicks(5u);
    EXPECT_EQ(0u                , Os_CounterControls[OS_COUNTER_SYSTEM].ticks) << "Counter wraps after maxallowedvalue";
    EXPECT_EQ(E_OK              , Os_GetAlarm_Internal(0, &tick));
    EXPECT_EQ(5u                , tick);
    EXPECT_EQ(E_OK              , Os_GetAlarm_Internal(1, &tick));
    EXPECT_EQ(50u               , tick);
    EXPECT_TRUE(Os_TestAdvance(0, 4u));
    Os_TestTicks(1u);
    EXPECT_EQ(E_OS_NOFUNC       , Os_GetAlarm_Internal(0, &tick));
}

TEST_F(Os_TestCounter, InvalidConfig) {
    Init(99u, 1u, 100u);
    EXPECT_FALSE(Os_Initialized) << "mincycle above maxallowedvalue";
    Init(99u, 0u, 1u);
    EXPECT_FALSE(Os_Initialized) << "ticksperbase of zero";
    Init(99u, 1u, 1u);
    EXPECT_TRUE(Os_Initialized);
    m_alarms[0].counter = OS_COUNTER_COUNT;
    Os_Init(&m_config);
    EXPECT_FALSE(Os_Initialized) << "Alarm on a counter that does not exist";
}

TEST_F(Os_TestCounter, RandomAgainstModelSmallCounter) {
    Init(99u, 1u, 3u);
    Os_TestRandomAlarms(99u, 3u);
}

TEST_F(Os_TestTask, AutostartPerMode) {
    Os_TaskStateType state;
    Os_TickType      tick;
    m_tasks[1].autostart  = OS_APPMODE_MASK(0u) | OS_APPMODE_MASK(2u);
    m_tasks[2].autostart  = OS_APPMODE_MASK(1u);
    m_alarms[0].autostart = OS_APPMODE_MASK(2u);
    m_alarms[0].alarmtime = 5u;
    m_alarms[0].cycletime = 3u;
    m_alarms[1].autostart = OS_APPMODE_MASK(0u);
    m_alarms[1].alarmtime = 7u;
    Os_Init(&m_config);
    ASSERT_TRUE(Os_Initialized);
    Os_Autostart(2u);
    EXPECT_EQ(E_OK              , Os_GetTaskState_Internal(1, &state));
    EXPECT_EQ(OS_TASK_READY     , state) << "Task 1 starts in mode 2";
    EXPECT_EQ(E_OK              , Os_GetTaskState_Internal(2, &state));
    EXPECT_EQ(OS_TASK_SUSPENDED , state) << "Task 2 only starts in mode 1";
    EXPECT_EQ(E_OK              , Os_GetAlarm_Internal(0, &tick)) << "Alarm 0 starts in mode 2";
    EXPECT_EQ(5u                , tick);
    EXPECT_EQ(E_OS_NOFUNC       , Os_GetAlarm_Internal(1, &tick)) << "Alarm 1 only starts in mode 0";
    Os_TestTicks(5u);
    EXPECT_EQ(E_OK              , Os_GetAlarm_Internal(0, &tick)) << "Autostarted cycle";
    EXPECT_EQ(3u                , tick);
}

TEST_F(Os_TestTask, AutostartAlarmInvalid) {
    m_alarms[0].autostart = OS_APPMODE_MASK(0u);
    m_alarms[0].alarmtime = 0u;
    Os_Init(&m_config);
    EXPECT_FALSE(Os_Initialized) << "Autostarted alarm needs an alarm time";
}

TEST_F(Os_TestTask, StartInvalidMode) {
    Os_Init(&m_config);
    ASSERT_TRUE(Os_Initialized);
    Os_ActiveTask = OS_INVALID_TASK;
    Os_Start(OS_APPMODE_COUNT);
    ASSERT_FALSE(Os_Errors.empty());
    EXPECT_EQ(E_OS_VALUE        , Os_Errors.top());
    EXPECT_EQ(OS_INVALID_TASK   , Os_ActiveTask) << "Does not start in an invalid mode";
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

static Os_StatusType Os_TestSyscall(Os_ServiceIdType service, Os_EventMaskType mask)
{
    Os_SyscallParamType param;
    param.service = service;
    param.p2.mask = mask;
    return Os_Syscall_Internal(&param);
}

TEST_F(Os_TestTask, WaitEventAlreadySet) {
    Os_EventMaskType mask;
    m_tasks[1].extended = TRUE;
    Start(1);
    EXPECT_EQ(E_OK             , Os_SetEvent_Internal(1, 0x2u));
    EXPECT_EQ(E_OK             , Os_TestSyscall(OSServiceId_WaitEvent, 0x6u));
    EXPECT_EQ(1                , Os_ActiveTask)              << "Event already set, keeps running";
    EXPECT_EQ(OS_TASK_RUNNING  , Os_TaskControls[1].state);
    EXPECT_EQ(E_OK             , Os_GetEvent_Internal(1, &mask));
    EXPECT_EQ(0x2u             , mask);
    EXPECT_EQ(E_OK             , Os_ClearEvent_Internal(0x2u));
    EXPECT_EQ(E_OK             , Os_GetEvent_Internal(1, &mask));
    EXPECT_EQ(0x0u             , mask);
}

TEST_F(Os_TestTask, WaitEventBlocks) {
    Os_EventMaskType mask;
    Os_TaskStateType state;
    m_tasks[2].extended = TRUE;
    Start(2);
    EXPECT_EQ(E_OK             , Os_ActivateTask_Internal(0));
    EXPECT_EQ(E_OK             , Os_TestSyscall(OSServiceId_WaitEvent, 0x1u));
    EXPECT_EQ(0                , Os_ActiveTask)              << "Next ready task runs while waiting";
    EXPECT_EQ(E_OK             , Os_GetTaskState_Internal(2, &state));
    EXPECT_EQ(OS_TASK_WAITING  , state);
    EXPECT_EQ(E_OK             , Os_SetEvent_Internal(2, 0x2u));
    EXPECT_EQ(0                , Os_ActiveTask)              << "Not an event it waits for";
    EXPECT_EQ(E_OK             , Os_SetEvent_Internal(2, 0x1u));
    EXPECT_EQ(2                , Os_ActiveTask)              << "Woken task preempts";
    EXPECT_EQ(OS_TASK_RUNNING  , Os_TaskControls[2].state);
    EXPECT_EQ(E_OK             , Os_GetEvent_Internal(2, &mask));
    EXPECT_EQ(0x3u             , mask);
}

TEST_F(Os_TestTask, WaitEventIdle) {
    m_tasks[2].extended = TRUE;
    Start(2);
    EXPECT_EQ(E_OK             , Os_TestSyscall(OSServiceId_WaitEvent, 0x1u));
    EXPECT_FALSE(Os_TaskRunning())                           << "Nothing runs";
    EXPECT_EQ(E_OK             , Os_SetEvent_Internal(2, 0x1u));
    EXPECT_EQ(2                , Os_ActiveTask);
    EXPECT_TRUE(Os_TaskRunning());
}

TEST_F(Os_TestTask, WaitEventErrors) {
    Os_EventMaskType mask;
    m_tasks[2].extended = TRUE;
    m_resources[1].priority = 3;
    Start(1);
    EXPECT_EQ(E_OS_ACCESS      , Os_TestSyscall(OSServiceId_WaitEvent, 0x1u)) << "Basic task cannot wait";
    EXPECT_EQ(E_OS_ACCESS      , Os_ClearEvent_Internal(0x1u))  << "Basic task has no events";
    EXPECT_EQ(E_OS_ACCESS      , Os_SetEvent_Internal(1, 0x1u)) << "Basic task has no events";
    EXPECT_EQ(E_OS_ACCESS      , Os_GetEvent_Internal(1, &mask));
    EXPECT_EQ(E_OS_STATE       , Os_SetEvent_Internal(2, 0x1u)) << "Suspended task";
    EXPECT_EQ(E_OS_STATE       , Os_GetEvent_Internal(2, &mask));
    EXPECT_EQ(E_OS_ID          , Os_SetEvent_Internal(OS_TASK_COUNT, 0x1u));
    EXPECT_EQ(E_OS_ID          , Os_GetEvent_Internal(OS_TASK_COUNT, &mask));

    EXPECT_EQ(E_OK             , Os_ActivateTask_Internal(2));
    EXPECT_EQ(2                , Os_ActiveTask);
    EXPECT_EQ(E_OK             , Os_GetResource_Internal(1));
    EXPECT_EQ(E_OS_RESOURCE    , Os_TestSyscall(OSServiceId_WaitEvent, 0x1u)) << "Cannot wait holding a resource";
    EXPECT_EQ(E_OK             , Os_ReleaseResource_Internal(1));

    Os_CallContext = OS_CONTEXT_ISR1;
    EXPECT_EQ(E_OS_CALLEVEL    , Os_TestSyscall(OSServiceId_WaitEvent, 0x1u)) << "Cannot wait in an interrupt";
    EXPECT_EQ(E_OS_CALLEVEL    , Os_ClearEvent_Internal(0x1u));
    Os_CallContext = OS_CONTEXT_TASK;
}

TEST_F(Os_TestTask, EventsClearedOnActivation) {
    Os_EventMaskType mask;
    m_tasks[2].extended = TRUE;
    Start(1);
    EXPECT_EQ(E_OK             , Os_ActivateTask_Internal(2));
    EXPECT_EQ(E_OK             , Os_SetEvent_Internal(2, 0x5u));
    EXPECT_EQ(E_OK             , Os_TerminateTask_Internal());
    EXPECT_EQ(E_OK             , Os_ActivateTask_Internal(2));
    EXPECT_EQ(E_OK             , Os_GetEvent_Internal(2, &mask));
    EXPECT_EQ(0x0u             , mask) << "Activation clears events";
}

TEST_F(Os_TestTask, AlarmSetsEvent) {
    m_tasks[2].extended = TRUE;
    m_alarms[0].task    = 2;
    m_alarms[0].event   = 0x4u;
    Start(2);
    EXPECT_EQ(E_OK             , Os_SetRelAlarm_Internal(0, 1u, 0u));
    EXPECT_EQ(E_OK             , Os_TestSyscall(OSServiceId_WaitEvent, 0x4u));
    EXPECT_FALSE(Os_TaskRunning());
    Os_Isr();
    EXPECT_EQ(2                , Os_ActiveTask)              << "Alarm event wakes the task";
    EXPECT_TRUE(Os_TaskRunning());
    EXPECT_EQ(1u               , Os_TaskControls[2].activation) << "Not activated again";
}

TEST_F(Os_TestTask, WaitEventInternalResource) {
    m_resources[1].priority = 3;
    m_tasks[1].resource     = 1;
    m_tasks[1].extended     = TRUE;
    Start(1);
    EXPECT_EQ(E_OK             , Os_ActivateTask_Internal(0));
    EXPECT_EQ(E_OK             , Os_TestSyscall(OSServiceId_WaitEvent, 0x1u));
    EXPECT_EQ(0                , Os_ActiveTask);
    EXPECT_EQ(OS_INVALID_RESOURCE, Os_TaskControls[1].resource) << "Released while waiting";
    EXPECT_EQ(E_OK             , Os_SetEvent_Internal(1, 0x1u));
    EXPECT_EQ(1                , Os_ActiveTask);
    EXPECT_EQ(1                , Os_TaskControls[1].resource) << "Taken again when it runs";
    EXPECT_EQ(3                , Os_TaskControls[1].priority);
}

TEST_F(Os_TestTask, ExtendedMultipleActivation) {
    m_tasks[1].extended   = TRUE;
    m_tasks[1].activation = 2u;
    Os_Init(&m_config);
    EXPECT_FALSE(Os_Initialized) << "Extended tasks cannot have multiple activations";
}
