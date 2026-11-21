add_executable(Os_MetricCooperative Os_Cfg.c)
target_include_directories(Os_MetricCooperative PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Os_MetricCooperative PRIVATE OsekOs::OsekOs)

# Runs for a few seconds and shuts the OS down, a crash or hang fails the test.
if(BUILD_TESTS)
    add_test(NAME Os_MetricCooperative COMMAND Os_MetricCooperative)
    set_tests_properties(Os_MetricCooperative PROPERTIES TIMEOUT 60)
endif()
