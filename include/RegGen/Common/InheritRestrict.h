#ifndef REGGEX_COMMON_INHERITRESTRICT_H
#define REGGEX_COMMON_INHERITRESTRICT_H

namespace RG {

class NonCopyable {
 public:
  NonCopyable() = default;
  ~NonCopyable() = default;

  NonCopyable(const NonCopyable&) = delete;
  auto operator=(const NonCopyable&) -> NonCopyable& = delete;
};

class NonMovable {
 public:
  NonMovable() = default;
  ~NonMovable() = default;

  NonMovable(NonMovable&&) = delete;
  auto operator=(NonMovable&&) -> NonMovable& = delete;
};

}  // namespace RG

#endif  // REGGEX_COMMON_INHERITRESTRICT_H