#include <af/ini_parse.hpp>

#include <cstdint>
#include <cstdio>
#include <cstdlib>

int
main(int argc, char** argv)
{
  if (argc < 2)
  {
    printf("Usage %s <path to ini>\n", argv[0]);
    return EXIT_FAILURE;
  }

  uint32_t keyValuePairs = 0;
  const auto sts = af::ini_read_and_parse(
    argv[1],
    [&keyValuePairs](af::string_view, af::string_view, af::string_view) -> bool
    {
      ++keyValuePairs;
      return true;
    });
  if (sts != af::ini_parse_status::ok)
  {
    printf("Error: %s\n", af::ini_status_string(sts));
    return EXIT_FAILURE;
  }

  printf("Summary: %u key/value pairs\n", keyValuePairs);

  return EXIT_SUCCESS;
}
