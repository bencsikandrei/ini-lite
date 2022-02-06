#pragma once

#include <string>
#include <system_error>

namespace af {

enum class ini_parse_errc {
  // The file doesn't exist
  invalid_file_path_non_existent = 1,
  // The file contains an un-closed section `[ ...`
  invalid_section_unmatched_token,
  // The file contains an empty section `[]`
  invalid_section_empty,
  // Empty key
  invalid_key_empty,
  // Empty value
  invalid_value_empty
};

namespace detail {

class ini_parse_error_category : public std::error_category {
public:
  [[nodiscard]] auto name() const noexcept -> const char * override {
    return "ini parse";
  }

  [[nodiscard]] auto message(int error) const noexcept -> std::string override {
    switch (static_cast<ini_parse_errc>(error)) {
    case ini_parse_errc::invalid_file_path_non_existent:
      return "Invalid file - doesn't exist";
    case ini_parse_errc::invalid_section_unmatched_token:
      return "Invalid INI section - unmatched `[`";
    case ini_parse_errc::invalid_section_empty:
      return "Invalid INI section - empty []";
    case ini_parse_errc::invalid_key_empty:
      return "Invalid INI key - empty `=...`";
    case ini_parse_errc::invalid_value_empty:
      return "Invalid INI key - empty `...=`";
    default:
      return "Unknown INI error";
    }
  }
};

} // namespace detail

/**
 * @brief Make `std::error_code` from an `af::ini_parse_errc`
 * @param error INI parse error
 * @return A `std::error_code` object
 */
inline auto make_error_code(ini_parse_errc error) noexcept -> std::error_code {
  static const detail::ini_parse_error_category category{};
  return std::error_code(static_cast<int>(error), category);
}

} // namespace af

namespace std {

template <>
struct is_error_code_enum<af::ini_parse_errc> : true_type {};

} // namespace std