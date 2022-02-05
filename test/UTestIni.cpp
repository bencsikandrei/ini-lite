#include <af/ini_parse.hpp>

#include <gtest/gtest.h>

#include <sstream>

namespace {

TEST(UTestIni, emptyFileDoesnCallCb) {
  std::stringstream input;
  bool called = false;
  ASSERT_FALSE(af::detail::readAndParse(
      input, [&called](std::string_view, std::string_view, std::string_view) {
        called = true;
        return true;
      }));
  ASSERT_FALSE(called);
}

TEST(UTestIni, justCategoryDoesNotCallCb) {
  std::stringstream input("[category]");
  bool called = false;
  ASSERT_FALSE(af::detail::readAndParse(
      input, [&called](std::string_view, std::string_view, std::string_view) {
        called = true;
        return true;
      }));
  ASSERT_FALSE(called);
}

TEST(UTestIni, unmatchedCategoryStart) {
  std::stringstream input("[");
  bool called = false;
  const std::error_code err = af::detail::readAndParse(
      input, [&called](std::string_view, std::string_view, std::string_view) {
        called = true;
        return true;
      });
  ASSERT_TRUE(err);
  ASSERT_EQ(static_cast<af::ini_parse_errc>(err.value()),
            af::ini_parse_errc::invalid_section_unmatched_token);
  ASSERT_FALSE(called);
}

TEST(UTestIni, emptyCategoryError) {
  std::stringstream input("[]");
  bool called = false;
  const std::error_code err = af::detail::readAndParse(
      input, [&called](std::string_view, std::string_view, std::string_view) {
        called = true;
        return true;
      });
  ASSERT_TRUE(err);
  ASSERT_EQ(static_cast<af::ini_parse_errc>(err.value()),
            af::ini_parse_errc::invalid_section_empty);
  ASSERT_FALSE(called);
}

TEST(UTestIni, onlyEqual) {
  std::stringstream input("=");
  bool called = false;
  const std::error_code err = af::detail::readAndParse(
      input, [&called](std::string_view, std::string_view, std::string_view) {
        called = true;
        return true;
      });
  ASSERT_TRUE(err);
  ASSERT_EQ(static_cast<af::ini_parse_errc>(err.value()),
            af::ini_parse_errc::invalid_key_empty);
  ASSERT_FALSE(called);
}

TEST(UTestIni, emptyKey) {
  std::stringstream input("=value");
  bool called = false;
  const std::error_code err = af::detail::readAndParse(
      input, [&called](std::string_view, std::string_view, std::string_view) {
        called = true;
        return true;
      });
  ASSERT_TRUE(err);
  ASSERT_EQ(static_cast<af::ini_parse_errc>(err.value()),
            af::ini_parse_errc::invalid_key_empty);
  ASSERT_FALSE(called);
}

TEST(UTestIni, singleKeyValueCallsWithEmptyCategory) {
  std::stringstream input("key=value");
  bool called = false;
  std::string category;
  std::string key;
  std::string value;
  ASSERT_FALSE(af::detail::readAndParse(
      input,
      [&](std::string_view c, std::string_view k, std::string_view v) -> bool {
        called = true;
        category = c;
        key = k;
        value = v;
        return true;
      }));
  ASSERT_TRUE(called);
  EXPECT_EQ(key, "key");
  EXPECT_EQ(value, "value");
  EXPECT_EQ(category, "");
}

TEST(UTestIni, singleKeyValueCallsWithCategory) {
  std::stringstream input(R"([category]    
key=value)");
  bool called = false;
  std::string category;
  std::string key;
  std::string value;
  ASSERT_FALSE(af::detail::readAndParse(
      input,
      [&](std::string_view c, std::string_view k, std::string_view v) -> bool {
        called = true;
        category = c;
        key = k;
        value = v;
        return true;
      }));
  ASSERT_TRUE(called);
  EXPECT_EQ(key, "key");
  EXPECT_EQ(value, "value");
  EXPECT_EQ(category, "category");
}

TEST(UTestIni, multipleValuesAndCategories) {
  std::stringstream input(R"([category1]
key=value1
[category2]
key2=value2
)");
  int called = 0;
  std::map<std::pair<std::string, std::string>, std::string> ini;

  ASSERT_FALSE(af::detail::readAndParse(
      input,
      [&](std::string_view c, std::string_view k, std::string_view v) -> bool {
        ++called;
        ini.emplace(std::make_pair(std::string(c), std::string(k)), v);
        return true;
      }));
  ASSERT_EQ(called, 2);

  const auto cat1kv =
      ini.find(std::make_pair(std::string("category1"), std::string("key")));
  ASSERT_NE(cat1kv, std::end(ini));
  ASSERT_EQ(cat1kv->second, "value1");

  const auto cat2kv =
      ini.find(std::make_pair(std::string("category2"), std::string("key2")));
  ASSERT_NE(cat2kv, std::end(ini));
  ASSERT_EQ(cat2kv->second, "value2");
}

} // namespace
