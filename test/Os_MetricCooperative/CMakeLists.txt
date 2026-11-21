add_executable(Os_MetricCooperative Os_Cfg.c)
target_include_directories(Os_MetricCooperative PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Os_MetricCooperative PRIVATE OsekOs::OsekOs)
