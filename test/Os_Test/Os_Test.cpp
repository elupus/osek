#include "gtest/gtest.h"
#include <stack>
#include <map>

extern "C" {
    #include "Std_Types.h"
    #include "Os.h"
    #include "Os_Cfg.h"
    #include "Os_Types.h"
}

struct Os_HooksInterface
{
    virtual ~Os_HooksInterface() {};
    virtual void ErrorHook(Os_StatusType ret)   {};
    virtual void PreTaskHook(Os_TaskType task)  {};
    virtual void PostTaskHook(Os_TaskType task) {};
};

Os_HooksInterface* Os_Hooks;

extern "C" void Os_ErrorHook   (Os_StatusType ret) { Os_Hooks->ErrorHook(ret);     }
extern "C" void Os_PreTaskHook (Os_TaskType task)  { Os_Hooks->PreTaskHook(task);  }
extern "C" void Os_PostTaskHook(Os_TaskType task)  { Os_Hooks->PostTaskHook(task); }


template<typename T> struct Os_Test : public testing::Test {
    typedef T ParentType;

    static T* active;

    virtual void SetUp()
    {
        memset(m_tasks    , 0, sizeof(m_tasks));
        memset(m_resources, 0, sizeof(m_resources));
        memset(m_alarms   , 0, sizeof(m_alarms));
        memset(m_task_activations, 0, sizeof(m_task_activations));
        for(Os_TaskType i = 0; i < OS_TASK_COUNT; ++i) {
            m_tasks[i].resource = OS_INVALID_RESOURCE;
        }
        for(Os_ResourceType i = 0; i < OS_RES_COUNT; ++i) {
            ;
        }
        for(Os_AlarmType i = 0; i < OS_ALARM_COUNT; ++i) {
            m_alarms[i].task    = OS_INVALID_TASK;
            m_alarms[i].counter = OS_COUNTER_SYSTEM;
        }

        m_resources[OS_RES_SCHEDULER].priority = OS_PRIO_COUNT;

        Os_Hooks           = &m_hooks;
        m_config.tasks     = &m_tasks;
        m_config.resources = &m_resources;
        m_config.alarms    = &m_alarms;
        active             = (T*)this;
    }

    virtual void TearDown()
    {
        Os_Hooks = NULL;
        for(Os_TaskType i = 0; i < OS_TASK_COUNT; ++i) {
            free(m_tasks[i].stack);
        }
    }

    struct Hooks : Os_HooksInterface {
        bool        shutdown;
        std::stack<Os_StatusType>  errors;
        Hooks() : shutdown(true)
        {}
        virtual ~Hooks()
        {}

        virtual void ErrorHook(Os_StatusType ret)
        {
            errors.push(ret);
            if(shutdown) {
                switch(ret) {
                case E_OS_LIMIT:
                    if ((Os_Error.service == OSServiceId_ActivateTask)
                    ||  (Os_Error.service == OSServiceId_ChainTask)) {
                        return; /* considered just warning */
                    }
                    break;
                case E_OS_NOFUNC:
                    if ((Os_Error.service == OSServiceId_GetAlarm)
                    ||  (Os_Error.service == OSServiceId_CancelAlarm)) {
                        return; /* considered just warning */
                    }
                    break;
                case E_OS_STATE:
                    if ((Os_Error.service == OSServiceId_SetAbsAlarm)
                    ||  (Os_Error.service == OSServiceId_SetRelAlarm)) {
                        return; /* considered just warning */
                    }
                    break;
                default:
                    break;
                }
                FAIL();
                Os_Shutdown();
            }
        }
    } m_hooks;

    typedef void (T::*task_entry_type)(void);
    std::map<Os_TaskType, task_entry_type> m_task_entries;

    static void task_entry(void)
    {
        Os_TaskType task;
        Os_GetTaskId(&task);
        active->m_task_activations[task]++;
        (active->*(active->m_task_entries[task]))();
        EXPECT_TRUE(false) << "Reached outside of task without terminate task: " << task;
        Os_TerminateTask();
    }

    void task_add(Os_TaskType id, task_entry_type entry, int autostart, Os_PriorityType priority, Os_ResourceType resource)
    {
        m_tasks[id].entry      = task_entry;
        m_tasks[id].stack_size = 8192*16;
        m_tasks[id].stack      = malloc(m_tasks[id].stack_size);
        m_tasks[id].autostart  = autostart;
        m_tasks[id].priority   = priority;
#if( (OS_CONFORMANCE == OS_CONFORMANCE_ECC2) ||  (OS_CONFORMANCE == OS_CONFORMANCE_BCC2) )
        m_tasks[id].activation = 255;
#endif
        m_tasks[id].resource   = resource;
        m_task_entries[id]     = entry;
    }

    unsigned int          m_task_activations[OS_TASK_COUNT];
    Os_TaskConfigType     m_tasks    [OS_TASK_COUNT];
    Os_ResourceConfigType m_resources[OS_RES_COUNT];
    Os_AlarmConfigType    m_alarms   [OS_ALARM_COUNT];
    Os_ConfigType         m_config;
};

template<typename T> T* Os_Test<T>::active = NULL;

struct Os_Test_Default : public Os_Test<Os_Test_Default>
{
    enum {
        OS_TASK_PRIO0,
        OS_TASK_PRIO1,
        OS_TASK_PRIO2,
        OS_TASK_PRIO3,
    };

    enum {
        OS_RES_OS = OS_RES_SCHEDULER,
        OS_RES_PRIO1,
        OS_RES_PRIO2,
        OS_RES_PRIO3,
        OS_RES_PRIO4,
    };

    void test_main(void)
    {
        task_add(OS_TASK_PRIO0, &ParentType::task_prio0, 1u, 0u, OS_INVALID_RESOURCE);
        task_add(OS_TASK_PRIO1, &ParentType::task_prio1, 0u, 1u, OS_INVALID_RESOURCE);
        task_add(OS_TASK_PRIO2, &ParentType::task_prio2, 0u, 2u, OS_INVALID_RESOURCE);
        task_add(OS_TASK_PRIO3, &ParentType::task_prio3, 0u, 3u, OS_INVALID_RESOURCE);
        m_resources[OS_RES_PRIO1].priority = 1;
        m_resources[OS_RES_PRIO2].priority = 2;
        m_resources[OS_RES_PRIO3].priority = 3;
        m_resources[OS_RES_PRIO4].priority = 4;

        m_hooks.shutdown = false;
        Os_Init(&m_config);
        Os_Start();
        EXPECT_GT(m_task_activations[OS_TASK_PRIO0], 0);
    }

    virtual void task_prio0(void)
    {
        Os_Shutdown();
    }

    virtual void task_prio1(void) { Os_TerminateTask(); }
    virtual void task_prio2(void) { Os_TerminateTask(); }
    virtual void task_prio3(void) { Os_TerminateTask(); }
};

struct Os_Test_ResourceOrder : public Os_Test_Default
{
    virtual void task_prio0(void)
    {
        EXPECT_EQ(E_OK       , Os_ActivateTask(OS_TASK_PRIO1));
        Os_Shutdown();
    }

    virtual void task_prio1(void)
    {
        /* check standard order */
        EXPECT_EQ(E_OK       , Os_GetResource    (OS_RES_PRIO1));
        EXPECT_EQ(E_OK       , Os_GetResource    (OS_RES_PRIO2));
        EXPECT_EQ(E_OK       , Os_ReleaseResource(OS_RES_PRIO2));
        EXPECT_EQ(E_OK       , Os_ReleaseResource(OS_RES_PRIO1));

        /* check invalid order */
        EXPECT_EQ(E_OK       , Os_GetResource    (OS_RES_PRIO2));
        EXPECT_EQ(E_OS_ACCESS, Os_GetResource    (OS_RES_PRIO1)) << "Lower prio resource grabbed";
        EXPECT_EQ(E_OS_NOFUNC, Os_ReleaseResource(OS_RES_PRIO1)) << "Non held resource released";
        EXPECT_EQ(E_OK       , Os_ReleaseResource(OS_RES_PRIO2));

        Os_TerminateTask();
    }
};

TEST_F(Os_Test_ResourceOrder, Main) {
    test_main();
}

struct Os_Test_ResourceTaskPriority : public Os_Test_Default
{
    virtual void task_prio0(void)
    {
        EXPECT_EQ(E_OK       , Os_ActivateTask(OS_TASK_PRIO2));
        Os_Shutdown();
    }

    virtual void task_prio2(void)
    {
        /* check standard order */
        EXPECT_EQ(E_OS_ACCESS, Os_GetResource    (OS_RES_PRIO1)) << "Lower prio resource grabbed in higher prio task";
        EXPECT_EQ(E_OK       , Os_GetResource    (OS_RES_PRIO2));
        EXPECT_EQ(E_OK       , Os_ReleaseResource(OS_RES_PRIO2));
        EXPECT_EQ(E_OS_NOFUNC, Os_ReleaseResource(OS_RES_PRIO1));
        Os_TerminateTask();
    }
};

TEST_F(Os_Test_ResourceTaskPriority, Main) {
    test_main();
    EXPECT_EQ(m_task_activations[OS_TASK_PRIO2], 1);
}

struct Os_Test_ResourceLockTest : public Os_Test_Default
{
    unsigned int task_prio1_count;
    Os_Test_ResourceLockTest()
    {
        task_prio1_count = 0;
    }

    virtual void task_prio0(void)
    {
        EXPECT_EQ(E_OK       , Os_ActivateTask(OS_TASK_PRIO1));
        EXPECT_EQ(E_OK       , Os_GetResource    (OS_RES_PRIO1));
        EXPECT_EQ(1          , task_prio1_count) << "Higher prio task has run more than once";
        EXPECT_EQ(E_OK       , Os_ActivateTask   (OS_TASK_PRIO1));
        EXPECT_EQ(1          , task_prio1_count) << "Higher prio task run while resource was held";
        EXPECT_EQ(E_OK       , Os_ReleaseResource(OS_RES_PRIO1));
        EXPECT_EQ(2          , task_prio1_count) << "Higher prio task didn't run after resource was released";
        Os_Shutdown();
    }

    virtual void task_prio1(void)
    {
        EXPECT_EQ(E_OK, Os_GetResource    (OS_RES_PRIO1));
        task_prio1_count++;
        EXPECT_EQ(E_OK, Os_ReleaseResource(OS_RES_PRIO1));
        Os_TerminateTask();
    }
};

TEST_F(Os_Test_ResourceLockTest, Main) {
    test_main();
    EXPECT_EQ(m_task_activations[OS_TASK_PRIO1], 2);
}

struct Os_Test_AlarmTest : public Os_Test_Default
{
    virtual void task_prio0(void)
    {
        Os_GetResource(OS_RES_PRIO4);
        EXPECT_NE(E_OK       , Os_SetRelAlarm(OS_ALARM_COUNT, 3, 0)) << "invalid alarm";

        /* setup high prio task running each cycle */
        EXPECT_EQ(E_OK       , Os_SetRelAlarm(2, 1, 1));

        /* setup two alarms running a bit later */
        EXPECT_EQ(E_OK       , Os_SetRelAlarm(0, 2, 0));
        EXPECT_EQ(E_OK       , Os_SetRelAlarm(1, 3, 0));

        /* trigger shutdown later */
        EXPECT_EQ(E_OK       , Os_SetRelAlarm(3, 4, 0));
        Os_ReleaseResource(OS_RES_PRIO4);

        while(1) {
            /* NOP */
        }
    }

    virtual void task_prio1(void)
    {
        EXPECT_EQ(m_task_activations[OS_TASK_PRIO1]+1, m_task_activations[OS_TASK_PRIO2]);
        EXPECT_EQ(E_OK       , Os_TerminateTask());
    }

    virtual void task_prio2(void)
    {
        EXPECT_EQ(E_OK       , Os_TerminateTask());
    }

    virtual void task_prio3(void)
    {
        Os_Shutdown();
    }
};

TEST_F(Os_Test_AlarmTest, Main) {
    m_alarms[0].task = OS_TASK_PRIO1;
    m_alarms[1].task = OS_TASK_PRIO1;
    m_alarms[2].task = OS_TASK_PRIO2;
    m_alarms[3].task = OS_TASK_PRIO3;
    test_main();
    EXPECT_EQ(m_task_activations[OS_TASK_PRIO1], 2);
    EXPECT_EQ(m_task_activations[OS_TASK_PRIO2], 3);
    EXPECT_EQ(m_task_activations[OS_TASK_PRIO3], 1);
}


struct Os_Test_AlarmTest2 : public Os_Test_Default
{
    virtual void task_prio0(void)
    {
        Os_GetResource(OS_RES_PRIO4);
        EXPECT_EQ(E_OS_ID    , Os_SetRelAlarm(OS_ALARM_COUNT, 3, 0)) << "invalid alarm";
        EXPECT_EQ(E_OS_VALUE , Os_SetRelAlarm(0, 0, 0))              << "invalid increment";
        EXPECT_EQ(E_OK       , Os_SetRelAlarm(0, 1, 1))              << "recurring alarm that will be cancelled";
        EXPECT_EQ(E_OK       , Os_SetRelAlarm(2, 4, 0))              << "alarm for shutdown";
        Os_ReleaseResource(OS_RES_PRIO4);

        while(1) {
            /* NOP */
        }
    }

    virtual void task_prio1(void)
    {
        EXPECT_EQ(E_OK       , Os_CancelAlarm(0));
        EXPECT_EQ(E_OK       , Os_TerminateTask());
    }

    virtual void task_prio2(void)
    {
        Os_Shutdown();
    }
};

TEST_F(Os_Test_AlarmTest2, Main) {
    m_alarms[0].task = OS_TASK_PRIO1;
    m_alarms[1].task = OS_TASK_PRIO1;
    m_alarms[2].task = OS_TASK_PRIO2;
    test_main();
    EXPECT_EQ(m_task_activations[OS_TASK_PRIO1], 1);
    EXPECT_EQ(m_task_activations[OS_TASK_PRIO2], 1);
}
