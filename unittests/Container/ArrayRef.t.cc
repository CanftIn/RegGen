#include "RegGen/Container/ArrayRef.h"

#include <gtest/gtest.h>

#include <limits>
#include <vector>

namespace RG {
namespace {

TEST(ArrayRefTest, DropBack) {
  static const int the_numbers[] = {4, 8, 15, 16, 23, 42};
  ArrayRef<int> arr1(the_numbers);
  ArrayRef<int> arr2(the_numbers, arr1.size() - 1);
  EXPECT_TRUE(arr1.drop_back().equals(arr2));
}

TEST(ArrayRefTest, DropFront) {
  static const int the_numbers[] = {4, 8, 15, 16, 23, 42};
  ArrayRef<int> arr1(the_numbers);
  ArrayRef<int> arr2(&the_numbers[2], arr1.size() - 2);
  EXPECT_TRUE(arr1.drop_front(2).equals(arr2));
}

TEST(ArrayRefTest, DropWhile) {
  static const int the_numbers[] = {1, 3, 5, 8, 10, 11};
  ArrayRef<int> arr1(the_numbers);
  ArrayRef<int> expected = arr1.drop_front(3);
  EXPECT_EQ(expected, arr1.drop_while([](const int& N) { return N % 2 == 1; }));

  EXPECT_EQ(arr1, arr1.drop_while([](const int& N) { return N < 0; }));
  EXPECT_EQ(ArrayRef<int>(),
            arr1.drop_while([](const int& N) { return N > 0; }));
}

TEST(ArrayRefTest, DropUntil) {
  static const int the_numbers[] = {1, 3, 5, 8, 10, 11};
  ArrayRef<int> arr1(the_numbers);
  ArrayRef<int> expected = arr1.drop_front(3);
  EXPECT_EQ(expected, arr1.drop_until([](const int& N) { return N % 2 == 0; }));

  EXPECT_EQ(ArrayRef<int>(),
            arr1.drop_until([](const int& N) { return N < 0; }));
  EXPECT_EQ(arr1, arr1.drop_until([](const int& N) { return N > 0; }));
}

TEST(ArrayRefTest, TakeBack) {
  static const int the_numbers[] = {4, 8, 15, 16, 23, 42};
  ArrayRef<int> arr1(the_numbers);
  ArrayRef<int> arr2(arr1.end() - 1, 1);
  EXPECT_TRUE(arr1.take_back().equals(arr2));
}

TEST(ArrayRefTest, TakeFront) {
  static const int the_numbers[] = {4, 8, 15, 16, 23, 42};
  ArrayRef<int> arr1(the_numbers);
  ArrayRef<int> arr2(arr1.data(), 2);
  EXPECT_TRUE(arr1.take_front(2).equals(arr2));
}

TEST(ArrayRefTest, TakeWhile) {
  static const int the_numbers[] = {1, 3, 5, 8, 10, 11};
  ArrayRef<int> arr1(the_numbers);
  ArrayRef<int> expected = arr1.take_front(3);
  EXPECT_EQ(expected, arr1.take_while([](const int& N) { return N % 2 == 1; }));

  EXPECT_EQ(ArrayRef<int>(),
            arr1.take_while([](const int& N) { return N < 0; }));
  EXPECT_EQ(arr1, arr1.take_while([](const int& N) { return N > 0; }));
}

TEST(ArrayRefTest, TakeUntil) {
  static const int the_numbers[] = {1, 3, 5, 8, 10, 11};
  ArrayRef<int> arr1(the_numbers);
  ArrayRef<int> expected = arr1.take_front(3);
  EXPECT_EQ(expected, arr1.take_until([](const int& N) { return N % 2 == 0; }));

  EXPECT_EQ(arr1, arr1.take_until([](const int& N) { return N < 0; }));
  EXPECT_EQ(ArrayRef<int>(),
            arr1.take_until([](const int& N) { return N > 0; }));
}

TEST(ArrayRefTest, Equals) {
  static const int a1[] = {1, 2, 3, 4, 5, 6, 7, 8};
  ArrayRef<int> arr1(a1);
  EXPECT_TRUE(arr1.equals({1, 2, 3, 4, 5, 6, 7, 8}));
  EXPECT_FALSE(arr1.equals({8, 1, 2, 4, 5, 6, 6, 7}));
  EXPECT_FALSE(arr1.equals({2, 4, 5, 6, 6, 7, 8, 1}));
  EXPECT_FALSE(arr1.equals({0, 1, 2, 4, 5, 6, 6, 7}));
  EXPECT_FALSE(arr1.equals({1, 2, 42, 4, 5, 6, 7, 8}));
  EXPECT_FALSE(arr1.equals({42, 2, 3, 4, 5, 6, 7, 8}));
  EXPECT_FALSE(arr1.equals({1, 2, 3, 4, 5, 6, 7, 42}));
  EXPECT_FALSE(arr1.equals({1, 2, 3, 4, 5, 6, 7}));
  EXPECT_FALSE(arr1.equals({1, 2, 3, 4, 5, 6, 7, 8, 9}));

  ArrayRef<int> arr1a = arr1.drop_back();
  EXPECT_TRUE(arr1a.equals({1, 2, 3, 4, 5, 6, 7}));
  EXPECT_FALSE(arr1a.equals({1, 2, 3, 4, 5, 6, 7, 8}));

  ArrayRef<int> arr1b = arr1a.slice(2, 4);
  EXPECT_TRUE(arr1b.equals({3, 4, 5, 6}));
  EXPECT_FALSE(arr1b.equals({2, 3, 4, 5, 6}));
  EXPECT_FALSE(arr1b.equals({3, 4, 5, 6, 7}));
}

TEST(ArrayRefTest, EmptyEquals) {
  EXPECT_TRUE(ArrayRef<unsigned>() == ArrayRef<unsigned>());
}

TEST(ArrayRefTest, ConstConvert) {
  int buf[4];
  for (int i = 0; i < 4; ++i) {
    buf[i] = i;
  }
  static int* A[] = {&buf[0], &buf[1], &buf[2], &buf[3]};
  ArrayRef<const int*> a((ArrayRef<int*>(A)));
  a = ArrayRef<int*>(A);
}

static auto ReturnTest12() -> std::vector<int> { return {1, 2}; }
static void ArgTest12(ArrayRef<int> a) {
  EXPECT_EQ(2U, a.size());
  EXPECT_EQ(1, a[0]);
  EXPECT_EQ(2, a[1]);
}

TEST(ArrayRefTest, InitializerList) {
  std::initializer_list<int> init_list = {0, 1, 2, 3, 4};
  ArrayRef<int> a = init_list;
  for (int i = 0; i < 5; ++i) {
    EXPECT_EQ(i, a[i]);
  }

  std::vector<int> b = ReturnTest12();
  a = b;
  EXPECT_EQ(1, a[0]);
  EXPECT_EQ(2, a[1]);

  ArgTest12({1, 2});
}

TEST(ArrayRefTest, EmptyInitializerList) {
  ArrayRef<int> a = {};
  EXPECT_TRUE(a.empty());

  a = {};
  EXPECT_TRUE(a.empty());
}

TEST(ArrayRefTest, ArrayRef) {
  static const int a1[] = {1, 2, 3, 4, 5, 6, 7, 8};

  // A copy is expected for non-const ArrayRef (thin copy)
  ArrayRef<int> arr1(a1);
  const ArrayRef<int>& arr1_ref = ArrayRef(arr1);
  EXPECT_NE(&arr1, &arr1_ref);
  EXPECT_TRUE(arr1.equals(arr1_ref));

  // A copy is expected for non-const ArrayRef (thin copy)
  const ArrayRef<int> arr2(a1);
  const ArrayRef<int>& arr2_ref = ArrayRef(arr2);
  EXPECT_NE(&arr2_ref, &arr2);
  EXPECT_TRUE(arr2.equals(arr2_ref));
}

}  // namespace
}  // namespace RG