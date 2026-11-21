add_executable(Os_MetricPreemptive Os_Cfg.c)
target_include_directories(Os_MetricPreemptive PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Os_MetricPreemptive PRIVATE OsekOs::OsekOs)
