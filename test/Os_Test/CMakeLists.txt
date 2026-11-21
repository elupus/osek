# Black box tests against the public OS API.
add_executable(Os_Test Os_Test.cpp)
target_include_directories(Os_Test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Os_Test PRIVATE OsekOs::OsekOs OsekOs::Coverage GTest::gtest_main)
set_target_properties(Os_Test PROPERTIES LINKER_LANGUAGE CXX)
gtest_discover_tests(Os_Test DISCOVERY_MODE PRE_TEST)
