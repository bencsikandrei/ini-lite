# Very (!) basic SAX style INI parser

c++17 INI parser with minimal set of features implemented:

    1. comments (on their own line) - NO inline comments supported
    2. sections (delimited by `[]`)
    3. key/value separated by `=`, duplicate keys allowed
    4. errors reported using `std::error_code`
    5. callback based section/key/value
    6. allows empty sections
    7. key/value must be non-empty
    8. NO UTF support (and not planned)

The goal was for it to be simple and fast.

## What can it parse?

A subset of INI as briefly described above.
There are limitations and it may not work for all your use cases.

## How to build?

It's header only.
Include `<af/ini_parse.hpp>`.

## CMake support

The library exports cmake config files `af-ini-config.cmake`/`af-ini-version.cmake`.
Install the library and set appropriate CMake paths to find it.

```
find_package(af-ini)
target_link_libraries(<your target> ... af-ini::af-ini)
```

## Dependencies

Unit tests require GTest 1.10.
There's a conan file that allows you to find that dependency.
You can also provide it in any other way, it must be called `....GTest` and must expose the following targets: `GTest::gtest_main`.

## CMake options

### Testing

Enable tests with `AF_INI_UNIT_TESTS` (default OFF) `AF_INI_INTEGRATION_TESTS` (default: OFF)

There's some data to play around with inside the `test/data` folder.
It may also help you figure out what this library supports.

### Benchmarks

Enable benchmarks with `AF_INI_BENCHMARKS` (default: OFF)

## Plans

No plans to develop this much, it's just a playground for testing some c++17 features.
Otherwise just use it as you wish it's as free as I can make it.
