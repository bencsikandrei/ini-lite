#include <af/ini_parse.hpp>

#include <cstdio>
#include <cstdlib>
#include <iostream>

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Usage %s <path to ini>\n", argv[0]);
    return EXIT_FAILURE;
  }

  const auto err = af::readAndParse(
      argv[1],
      [](std::string_view s, std::string_view k, std::string_view v) -> bool {
        std::cout << "Section: " << s << " | key: '" << k << "' value: '" << v
                  << "'\n";
        return true;
      });
  if (err) {
    std::cout << "Error: " << err.message() << '\n';
    return EXIT_FAILURE;
  }
}