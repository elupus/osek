# White box tests; Os_TestInternal.cpp includes Os.c directly.
add_executable(Os_TestInternal Os_TestInternal.cpp)
target_include_directories(Os_TestInternal PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Os_TestInternal PRIVATE OsekOs::Headers OsekOs::Coverage GTest::gtest_main)
set_target_properties(Os_TestInternal PROPERTIES LINKER_LANGUAGE CXX)
gtest_discover_tests(Os_TestInternal DISCOVERY_MODE PRE_TEST)
