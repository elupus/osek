add_executable(Os_MetricEvents Os_Cfg.c)
target_include_directories(Os_MetricEvents PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Os_MetricEvents PRIVATE OsekOs::OsekOs)

# Runs for a few seconds and shuts the OS down, a crash or hang fails the test.
if(BUILD_APP_TESTS)
    add_test(NAME Os_MetricEvents COMMAND Os_MetricEvents)
    set_tests_properties(Os_MetricEvents PROPERTIES TIMEOUT 60)
endif()
