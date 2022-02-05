find_package(GTest REQUIRED)

add_executable(UTestIni UTestIni.cpp)
target_link_libraries(UTestIni PRIVATE af-ini::af-ini GTest::gtest_main)
add_test(NAME UTestIni COMMAND UTestIni)

add_executable(ITestIni ITestIni.cpp)
target_link_libraries(ITestIni PRIVATE af-ini::af-ini)
