add_executable(Os_MetricPreemptive Os_Cfg.c)
target_include_directories(Os_MetricPreemptive PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Os_MetricPreemptive PRIVATE OsekOs::OsekOs)

# Runs for a few seconds and shuts the OS down, a crash or hang fails the test.
if(BUILD_APP_TESTS)
    add_test(NAME Os_MetricPreemptive COMMAND Os_MetricPreemptive)
    set_tests_properties(Os_MetricPreemptive PROPERTIES TIMEOUT 60)
endif()
