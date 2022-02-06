#include <af/ini_parse.hpp>

#include <cstdint>
#include <cstdio>
#include <cstdlib>

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Usage %s <path to ini>\n", argv[0]);
    return EXIT_FAILURE;
  }

  uint32_t keyValuePairs = 0;
  const auto err =
      af::read_and_parse(argv[1],
                         [&keyValuePairs](std::string_view, std::string_view,
                                          std::string_view) -> bool {
                           ++keyValuePairs;
                           return true;
                         });
  if (err) {
    printf("Error: %s\n", err.message().c_str());
    return EXIT_FAILURE;
  }

  printf("Summary: %u key/value pairs\n", keyValuePairs);

  return EXIT_SUCCESS;
}
