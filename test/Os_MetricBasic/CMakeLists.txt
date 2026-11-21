add_executable(Os_MetricBasic Os_Cfg.c)
target_include_directories(Os_MetricBasic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Os_MetricBasic PRIVATE OsekOs::OsekOs)

# Runs for a few seconds and shuts the OS down, a crash or hang fails the test.
if(BUILD_APP_TESTS)
    add_test(NAME Os_MetricBasic COMMAND Os_MetricBasic)
    set_tests_properties(Os_MetricBasic PROPERTIES TIMEOUT 60)
endif()
