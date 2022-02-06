#include <af/ini_parse.hpp>

#include <cstdio>
#include <cstdlib>
#include <iostream>

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Usage %s <path to ini>\n", argv[0]);
    return EXIT_FAILURE;
  }

  const auto err = af::read_and_parse(
      argv[1],
      [](std::string_view s, std::string_view k, std::string_view v) -> bool {
        std::cout << "Section: " << s << " | key: '" << k << "' value: '" << v
                  << "'\n";
        return true;
      });
  if (err) {
    printf("Error: %s\n", err.message().c_str());
    return EXIT_FAILURE;
  }
}
