#include "RegGen/Lexer/Regex.h"

#include <gtest/gtest.h>

namespace RG {
namespace {

TEST(Regex, Basic) {
  std::string regex = "a|b";

  auto root = ParseRegex(regex);
}

TEST(Regex, Alnum) {
  std::string regex1 = "[A-Za-z0-9]";
  std::string regex2 = "[^A-F(a|f)^0-9]";

  auto root1 = ParseRegex(regex1);
  auto root2 = ParseRegex(regex2);
}

}  // namespace

}  // namespace RG