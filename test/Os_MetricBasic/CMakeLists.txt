add_executable(Os_MetricBasic Os_Cfg.c)
target_include_directories(Os_MetricBasic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Os_MetricBasic PRIVATE OsekOs::OsekOs)
