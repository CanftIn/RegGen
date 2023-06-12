#include "RegGen/Common/Text.h"

#include <gtest/gtest.h>

namespace RG {
namespace {

TEST(Text, Consume) {
  const char* str = "hello world";

  EXPECT_TRUE(Consume(str) == 'h');
  EXPECT_TRUE(Consume(str) == 'e');
  EXPECT_TRUE(ConsumeIf(str, 'l'));
  EXPECT_TRUE(ConsumeIf(str, "lo"));
  EXPECT_FALSE(ConsumeIf(str, 'a', 'z'));
  EXPECT_TRUE(ConsumeIfAny(str, "\t\r\n "));
}

}  // namespace

}  // namespace RG