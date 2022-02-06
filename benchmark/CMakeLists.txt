find_package(benchmark REQUIRED)

add_executable(af-ini-benchmarks af-ini-benchmarks.cpp)
target_link_libraries(af-ini-benchmarks
PRIVATE
  af-ini::af-ini
  benchmark::benchmark
)
