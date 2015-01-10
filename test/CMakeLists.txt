set(osekos_test_sources
    osekos_test_main.cpp
)

include_directories(${GTEST_INCLUDE_DIR})

add_executable(osekos_test ${osekos_test_sources})
target_link_libraries(osekos_test gtest gtest_main)
