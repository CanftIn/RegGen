#include "RegGen/Common/Format.h"

#include <gtest/gtest.h>

#include <string>

namespace RG {
namespace {

TEST(Format, Basic) {
  {
    std::string expected = "a(1,2.2,3,\"4\")";
    std::string yield = Format("a({},{},{},{})", 1, 2.2, '3', "\"4\"");

    EXPECT_TRUE(expected == yield);
  }

  {
    std::string expected = "{text}";
    std::string yield = Format("{{{0}}}", "text");

    EXPECT_TRUE(expected == yield);
  }

  {
    std::string expected = "{foo, baz";
    std::string yield = Format("{{{0}, {2}", "foo", "bar", "baz");

    EXPECT_TRUE(expected == yield);
  }

  {
    std::string expected = "test-332211";
    std::string yield = Format("test-{2}{1}{0}", 11, 22, 33);

    EXPECT_TRUE(expected == yield);
  }

  {
    std::string expected = "112211!!";
    std::string yield = Format("{}{}{0}!!", 11, 22);

    EXPECT_TRUE(expected == yield);
  }
}

}  // namespace
}  // namespace RG