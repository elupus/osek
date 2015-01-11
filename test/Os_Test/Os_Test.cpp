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
        for(Os_TaskType i = 0; i < OS_TASK_COUNT; ++i) {
            m_tasks[i].resource = OS_INVALID_RESOURCE;
        }
        for(Os_ResourceType i = 0; i < OS_RES_COUNT; ++i) {
            ;
        }
        for(Os_AlarmType i = 0; i < OS_ALARM_COUNT; ++i) {
            m_alarms[i].task = OS_INVALID_TASK;
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
        (active->*(active->m_task_entries[task]))();
    }

    void task_add(Os_TaskType id, task_entry_type entry, int autostart, Os_PriorityType priority, Os_ResourceType resource)
    {
        m_tasks[id].entry      = task_entry;
        m_tasks[id].stack_size = 8192*16;
        m_tasks[id].stack      = malloc(m_tasks[id].stack_size);
        m_tasks[id].autostart  = autostart;
        m_tasks[id].priority   = priority;
        m_tasks[id].resource   = resource;
        m_task_entries[id]     = entry;
    }

    Os_TaskConfigType     m_tasks    [OS_TASK_COUNT];
    Os_ResourceConfigType m_resources[OS_RES_COUNT];
    Os_AlarmConfigType    m_alarms   [OS_ALARM_COUNT];
    Os_ConfigType         m_config;
};

template<typename T> T* Os_Test<T>::active = NULL;

struct Os_Test_Simple : public Os_Test<Os_Test_Simple>
{
    int           idle_count;
    int           busy_count;

    enum {
        OS_TASK_IDLE,
        OS_TASK_BUSY,
    };

    enum {
        OS_RES_OS = OS_RES_SCHEDULER,
        OS_RES_LOCK,
    };

    void test_main(void)
    {
        task_add(OS_TASK_IDLE, &ParentType::idle_main, 1u, 0u, OS_INVALID_RESOURCE);
        task_add(OS_TASK_BUSY, &ParentType::busy_main, 0u, 1u, OS_INVALID_RESOURCE);
        m_resources[OS_RES_LOCK].priority = 2;

        m_hooks.shutdown = false;
        idle_count = 0;
        busy_count = 0;
        Os_Init(&m_config);
        Os_Start();
        EXPECT_EQ(m_hooks.errors.size(), 1);
        EXPECT_GT(idle_count, 0);
    }

    void idle_main(void)
    {
        idle_count++;
        EXPECT_EQ(Os_ActivateTask(OS_TASK_COUNT), E_OS_ID);
        EXPECT_EQ(Os_GetResource(OS_RES_LOCK), E_OK);
        EXPECT_EQ(Os_ActivateTask(OS_TASK_BUSY), E_OK); /* first  activation */
        EXPECT_EQ(Os_ActivateTask(OS_TASK_BUSY), E_OK); /* second activation */
        EXPECT_EQ(busy_count, 0);                       /* we hold resource so we block task */
        EXPECT_EQ(Os_ReleaseResource(OS_RES_LOCK), E_OK);

        /* this should schedule busy instead */
        EXPECT_EQ(busy_count, 2);

        Os_Shutdown();
        Os_TerminateTask();
    }

    void busy_main(void)
    {
        EXPECT_EQ(Os_GetResource(OS_RES_LOCK), E_OK);
        busy_count++;
        EXPECT_EQ(Os_ReleaseResource(OS_RES_LOCK), E_OK);
        Os_TerminateTask();
    }
};

TEST_F(Os_Test_Simple, Main) {
    test_main();
}



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
        task_add(OS_TASK_PRIO1, &ParentType::task_prio1, 1u, 1u, OS_INVALID_RESOURCE);
        task_add(OS_TASK_PRIO2, &ParentType::task_prio2, 1u, 2u, OS_INVALID_RESOURCE);
        task_add(OS_TASK_PRIO3, &ParentType::task_prio3, 1u, 3u, OS_INVALID_RESOURCE);
        m_resources[OS_RES_PRIO1].priority = 1;
        m_resources[OS_RES_PRIO2].priority = 2;
        m_resources[OS_RES_PRIO3].priority = 3;
        m_resources[OS_RES_PRIO4].priority = 4;

        m_hooks.shutdown = false;
        Os_Init(&m_config);
        Os_Start();
    }

    virtual void task_prio0(void)
    {
        Os_Shutdown();
        Os_TerminateTask();
    }

    virtual void task_prio1(void) { Os_TerminateTask(); }
    virtual void task_prio2(void) { Os_TerminateTask(); }
    virtual void task_prio3(void) { Os_TerminateTask(); }
};

struct Os_Test_ResourceOrder : public Os_Test_Default
{
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

struct Os_Test_ResourceTask : public Os_Test_Default
{
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

TEST_F(Os_Test_ResourceTask, Main) {
    test_main();
}
