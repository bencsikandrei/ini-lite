add_library(af-ini-test-interface INTERFACE)
target_compile_options(af-ini-test-interface INTERFACE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Wno-unused-but-set-variable -Werror>
)

if(AF_INI_UNIT_TESTS)
  # unit
  find_package(GTest 1.10 REQUIRED)

  add_executable(af-ini-unit-tests unit/af-ini-unit-tests.cpp)
  target_link_libraries(af-ini-unit-tests
    PRIVATE
      af-ini::af-ini
      af-ini-test-interface
      GTest::gtest_main
  )
  add_test(NAME af-ini-unit-tests COMMAND af-ini-unit-tests)
endif()

if(AF_INI_INTEGRATION_TESTS)
  # integration
  add_executable(af-ini-integration-tests integration/af-ini-integration-tests.cpp)
  target_link_libraries(af-ini-integration-tests
    PRIVATE
      af-ini::af-ini
      af-ini-test-interface
  )
  add_test(NAME af-ini-integration-tests COMMAND af-ini-integration-tests ${CMAKE_CURRENT_LIST_DIR}/data/valid1.ini)

  add_executable(af-ini-benchmark-ish integration/af-ini-benchmark-ish.cpp)
  # target_compile_options(af-ini-benchmark-ish PRIVATE 
  #   $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-march=native>
  # )
  target_link_libraries(af-ini-benchmark-ish
    PRIVATE
      af-ini::af-ini
      af-ini-test-interface
  )
  add_test(NAME af-ini-benchmark-ish COMMAND af-ini-benchmark-ish ${CMAKE_CURRENT_LIST_DIR}/data/valid1.ini)
endif()
