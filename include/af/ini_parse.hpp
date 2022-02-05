#pragma once

#include "ini_error.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string_view>
#include <system_error>
#include <utility>

#include <cassert>

// temporary
#include <iostream>

namespace af {

namespace detail {

template <typename Func>
struct ScopeGuard {
public:
  ScopeGuard() noexcept { mAtExit(); }

  template <typename AtExit>
  ScopeGuard(AtExit &&atexit) : mAtExit(std::forward<AtExit>(atexit)) {}

private:
  Func mAtExit;
};

template <typename AtExit>
static ScopeGuard<AtExit> makeScopeGuard(AtExit &&atexit) {
  return ScopeGuard<AtExit>(std::forward<AtExit>(atexit));
}

/**
 * @brief Strip spaces of the right of a string
 */
std::string_view rtrim(std::string_view s) {
  s.remove_suffix(std::distance(
      s.crbegin(), std::find_if(s.crbegin(), s.crend(),
                                [](int c) { return !std::isspace(c); })));
  return s;
}

/**
 * @brief Strip spaces of the left of a string
 */
std::string_view ltrim(std::string_view s) noexcept {
  s.remove_prefix(
      std::distance(s.cbegin(), std::find_if(s.cbegin(), s.cend(), [](int c) {
                      return !std::isspace(c);
                    })));
  return s;
}

constexpr bool startsWith(std::string_view v,
                          std::string_view prefix) noexcept {
  return v.substr(0, prefix.size()) == prefix;
}

constexpr bool startsWith(std::string_view v, char c) noexcept {
  return !v.empty() && v.front() == c;
}

constexpr bool startsWithUnchecked(std::string_view v, char c) noexcept {
  assert(!v.empty() && "Don't pass empty view to startsWithUnchecked");
  return v.front() == c;
}

constexpr bool endsWithUnchecked(std::string_view v, char c) noexcept {
  assert(!v.empty() && "Don't pass empty view to endsWithUnchecked");
  return v.back() == c;
}

template <typename ValueCallback, typename std::enable_if_t<true, bool> = false>
std::error_code readAndParse(std::istream &input, ValueCallback &&cb) {
  static constexpr uint16_t kCategoryMaxLen = 512;
  static constexpr uint16_t kKeyMaxLen = 256;
  static constexpr uint16_t kValueMaxLen = 1791;
  static constexpr uint16_t kBufferMaxLen = 2048;

  uint16_t categoryLen = 0;
  char category[kCategoryMaxLen];

  for (char a[kBufferMaxLen]; input.getline(&a[0], kBufferMaxLen - 1);) {
    // let's remove the cruft after the read buffer when we move to the next
    // line
    auto _ = makeScopeGuard([&input]() {
      input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    });

    std::string_view line(ltrim(std::string_view(&a[0])));

    const auto isComment = [](std::string_view strippedLine) {
      static constexpr std::string_view kCommentStarters = "#";
      return startsWith(strippedLine, kCommentStarters);
    };

    if (isComment(line)) {
      continue;
    }

    line = rtrim(line);
    if (line.empty()) {
      continue;
    }

    const auto isSectionStart = [](std::string_view trimmedLine) {
      static constexpr char kSectionStarter = '[';
      return startsWithUnchecked(trimmedLine, kSectionStarter);
    };

    // check if section [<section_name>]
    if (isSectionStart(line)) {
      // check if has matching ']'
      static constexpr char kSectionEnder = ']';
      if (!endsWithUnchecked(line, kSectionEnder)) {
        return make_error_code(ini_parse_errc::invalid_section_unmatched_token);
      }
      if (line.size() < 3) {
        return make_error_code(ini_parse_errc::invalid_section_empty);
      }
      // valid section, may still be empty, keep it in the buffer
      std::copy(std::next(std::cbegin(line)), std::prev(std::cend(line)),
                std::begin(category));
      categoryLen = static_cast<uint16_t>(line.length() - 2);
      continue;
    }

    // we can now check for key / value pairs with '=' in between
    const auto posOfEqual = line.find_first_of('=');
    std::string_view k = line.substr(0, posOfEqual);
    // we have a key, but do we have a value?
    if (k.empty()) {
      return make_error_code(ini_parse_errc::invalid_key_empty);
    }

    // key is not empty, how about value?
    std::string_view v = line.substr(k.length() + 1);
    if (v.empty()) {
      return make_error_code(ini_parse_errc::invalid_value_empty);
    }

    // we got them all, call back
    if (!cb(std::string_view(category, categoryLen), k, v)) {
      break;
    }
  }

  return {};
}

} // namespace detail

/**
 * @brief Calls the user provided callback whenever a key/value is available
 * @pre .ini file with value len < 1791, key len < 256, section len < 512
 * @param path .ini file path
 * @param cb user provided (with optional state) callback
 * @return error code describind the success/failure
 */
template <typename ValueCallback, typename std::enable_if_t<true, bool> = false>
std::system_error readAndParse(std::filesystem::path const &path,
                               ValueCallback &&cb) {
  if (std::error_code ec; !std::filesystem::exists(path, ec)) {
    return ec;
  }

  std::ifstream file(path);
  if (!file.good()) {
    return std::make_error_code(std::errc::io_error);
  }

  return readAndParse(file, std::forward<ValueCallback>(cb));
}

} // namespace af
