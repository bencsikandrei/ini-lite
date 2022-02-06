#include <af/ini_parse.hpp>

#include <benchmark/benchmark.h>

#include <sstream>

static void IniParserBenchmark_allCases(benchmark::State &state) {
  std::stringstream ini(R"(# a comment
[blanks]
one=two
a=  1
b=      1   
c       =1
d =         1

; another comment   
e =1        
f = 1   
g =1      
h =1)");
  int count = 0;
  for (auto _ : state) {
    af::detail::read_and_parse(
        ini,
        [&count](std::string_view, std::string_view, std::string_view) -> bool {
          ++count;
          return true;
        });
  }
  benchmark::DoNotOptimize(count);
  if (count != 9) {
    std::abort();
  }
}

BENCHMARK(IniParserBenchmark_allCases);
