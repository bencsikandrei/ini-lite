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

namespace af {

namespace detail {

/**
 * @brief Defer something till the end of a scope
 */
template <typename Func>
struct [[nodiscard]] scope_guard {
public:
  scope_guard() noexcept { mAtExit(); }

  template <typename AtExit>
  scope_guard(AtExit &&atexit) : mAtExit(std::forward<AtExit>(atexit)) {}

private:
  Func mAtExit;
};

template <typename AtExit>
static scope_guard<AtExit> make_scope_guard(AtExit &&atexit) {
  return scope_guard<AtExit>(std::forward<AtExit>(atexit));
}

[[nodiscard]] inline bool isspace(char c) noexcept {
  return c == ' ' || c == '\t';
}

/**
 * @brief Strip spaces of the right of a string
 */
std::string_view rtrim(std::string_view s) {
  s.remove_suffix(std::distance(
      s.crbegin(),
      std::find_if(s.crbegin(), s.crend(), [](int c) { return !isspace(c); })));
  return s;
}

/**
 * @brief Strip spaces of the left of a string
 */
std::string_view ltrim(std::string_view s) noexcept {
  s.remove_prefix(std::distance(
      s.cbegin(),
      std::find_if(s.cbegin(), s.cend(), [](int c) { return !isspace(c); })));
  return s;
}

/**
 * @brief Trim on both ends
 */
[[nodiscard]] std::string_view trim(std::string_view s) noexcept {
  return ltrim(rtrim(s));
}

/**
 * @brief Check if a string view starts with a given prefix
 */
[[nodiscard]] constexpr bool starts_with(std::string_view v,
                                         std::string_view prefix) noexcept {
  return v.substr(0, prefix.size()) == prefix;
}

/**
 * @brief Check if a string view starts with a given char
 */
[[nodiscard]] constexpr bool starts_with(std::string_view v, char c) noexcept {
  return !v.empty() && v.front() == c;
}

/**
 * @brief Check if a string view starts with a given char
 * @pre the string_view mustn't be empty
 */
[[nodiscard]] constexpr bool starts_with_unchecked(std::string_view v,
                                                   char c) noexcept {
  assert(!v.empty() && "Don't pass empty view to starts_with_unchecked");
  return v.front() == c;
}

/**
 * @brief Check if a string view ends with a given char
 * @pre the string_view mustn't be empty
 */
[[nodiscard]] constexpr bool ends_with_unchecked(std::string_view v,
                                                 char c) noexcept {
  assert(!v.empty() && "Don't pass empty view to ends_with_unchecked");
  return v.back() == c;
}

/**
 * @brief Parse data from an istream
 * The user provided callback must return a bool indicating if parsing should
 * continue
 * @param input stream to parse
 * @param cb user provided callback for (section, key, value) -> bool
 */
template <typename ValueCallback, typename std::enable_if_t<true, bool> = false>
[[nodiscard]] std::error_code read_and_parse(std::istream &input,
                                             ValueCallback &&cb) {
  static constexpr uint16_t kCategoryMaxLen = 512;
  static constexpr uint16_t kBufferMaxLen = 2048;

  uint16_t sectionLen = 0;
  char section[kCategoryMaxLen];

  for (char a[kBufferMaxLen]; input.getline(&a[0], kBufferMaxLen - 1);) {
    std::string_view line(ltrim(std::string_view(&a[0])));
    if (line.empty()) {
      continue;
    }

    // let's remove the cruft after the read buffer when we move to the next
    // line
    auto deferClear = make_scope_guard([&input]() {
      input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    });

    const auto isComment = [](std::string_view strippedLine) {
      static constexpr std::string_view kCommentHash = "#";
      static constexpr std::string_view kCommentSemi = ";";
      return starts_with(strippedLine, kCommentHash) ||
             starts_with(strippedLine, kCommentSemi);
    };

    if (isComment(line)) {
      continue;
    }

    const auto isSectionStart = [](std::string_view trimmedLine) {
      static constexpr char kSectionStarter = '[';
      return starts_with_unchecked(trimmedLine, kSectionStarter);
    };

    // check if section [<section_name>]
    if (isSectionStart(line)) {
      line = rtrim(line);
      // check if has matching ']'
      static constexpr char kSectionEnder = ']';
      if (!ends_with_unchecked(line, kSectionEnder)) {
        return make_error_code(ini_parse_errc::invalid_section_unmatched_token);
      }
      if (line.size() < 3) {
        return make_error_code(ini_parse_errc::invalid_section_empty);
      }
      // valid section, may still be empty, keep it in the buffer
      std::copy(std::next(std::cbegin(line)), std::prev(std::cend(line)),
                std::begin(section));
      sectionLen = static_cast<uint16_t>(line.length() - 2);
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
    if (!cb(trim(std::string_view(section, sectionLen)), trim(k), trim(v))) {
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
[[nodiscard]] std::error_code read_and_parse(std::filesystem::path const &path,
                                             ValueCallback &&cb) {
  if (std::error_code ec; !std::filesystem::exists(path, ec)) {
    if (ec) {
      return ec;
    }
    return make_error_code(ini_parse_errc::invalid_file_path_non_existent);
  }

  if (std::ifstream file(path); !file.good()) {
    return std::make_error_code(std::errc::io_error);
  } else {
    return detail::read_and_parse(file, std::forward<ValueCallback>(cb));
  }
}

} // namespace af
