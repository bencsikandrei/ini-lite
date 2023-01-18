#include <af/ini_parse.hpp>

#include <cstdio>
#include <cstdlib>
#include <iostream>

int
main(int argc, char** argv)
{
  if (argc < 2)
  {
    printf("Usage %s <path to ini>\n", argv[0]);
    return EXIT_FAILURE;
  }

  const auto sts = af::ini_read_and_parse(
    argv[1],
    [](af::string_view, af::string_view k, af::string_view v) -> bool
    {
      ::fwrite(k.data(), sizeof(char), k.size(), stdout);
      ::puts("");
      ::fwrite(v.data(), sizeof(char), v.size(), stdout);
      return true;
    });
  if (sts != af::ini_parse_status::ok)
  {
    printf("Error: %s\n", af::ini_status_string(sts));
    return EXIT_FAILURE;
  }
}
