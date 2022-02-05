#include <af/Ini.hpp>

#include <algorithm>
#include <cctype>

namespace {

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
std::string_view ltrim(std::string_view s) {
  s.remove_prefix(
      std::distance(s.cbegin(), std::find_if(s.cbegin(), s.cend(), [](int c) {
                      return !std::isspace(c);
                    })));
  return s;
}

} // namespace

namespace af {} // namespace af
