add_executable(Os_Run Os_Cfg.c)
target_include_directories(Os_Run PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Os_Run PRIVATE OsekOs::OsekOs)
