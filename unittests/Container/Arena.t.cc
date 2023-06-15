#include "RegGen/Container/Arena.h"

#include <gtest/gtest.h>

namespace RG {
namespace {

class Inc {
 public:
  explicit Inc(int* p) : p_(p) { *p_ += 1; }

  ~Inc() { *p_ -= 1; }

 private:
  int* p_;
};

void DoAllocTest(Arena& arena, int times) {
  for (int i = 0; i < times; ++i) {
    arena.Allocate(rand() % 3000);
  }
}

TEST(Arena, POD) {
  Arena arena;
  int* p1 = arena.Construct<int>(42);
  auto* p2 = arena.Construct<float>(3.14F);
  EXPECT_TRUE(*p1 == 42);
  EXPECT_TRUE(*p2 == 3.14F);
  EXPECT_TRUE(arena.GetByteUsed() == sizeof(int*) + sizeof(float*));
}

TEST(Arena, Destructor) {
  int count = 0;

  {
    Arena arena;
    auto* inc1 = arena.Construct<Inc>(&count);
    auto* inc2 = arena.Construct<Inc>(&count);
    EXPECT_TRUE(count == 2);
  }
  EXPECT_TRUE(count == 0);
}

TEST(Arena, RandomAlloc) {
  Arena arena;
  EXPECT_NO_THROW(DoAllocTest(arena, 1000));
}

}  // namespace
}  // namespace RG