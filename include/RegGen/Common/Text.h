#ifndef REGGEN_COMMON_TEXT_H
#define REGGEN_COMMON_TEXT_H

#include <algorithm>
#include <cassert>

namespace RG {

// [a-zA-Z]
inline auto IsAlpha(char c) -> bool {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

// [0-9]
inline auto IsDigit(char c) -> bool { return c >= '0' && c <= '9'; }

// [a-zA-Z0-9]
inline auto IsAlnum(char c) -> bool { return IsAlpha(c) || IsDigit(c); }

// [a-z]
inline auto IsLower(char c) -> bool { return 'a' <= c && c <= 'z'; }

inline auto Consume(const char*& s) -> char {
  assert(*s);
  return *(s++);
}

inline auto ConsumeIf(const char*& s, char c) -> bool {
  assert(c != '\0');
  if (*s == c) {
    ++s;
    return true;
  }
  return false;
}

inline auto ConsumeIf(const char*& s, const char* pred) -> bool {
  assert(pred != nullptr);
  const char* p = s;
  while (*pred) {
    if (*p != *pred) {
      return false;
    }
    ++p;
    ++pred;
  }
  s = p;
  return true;
}

inline auto ConsumeIf(const char*& s, int begin, int end) -> bool {
  assert(begin <= end && begin > 0);
  if (begin <= *s && *s <= end) {
    ++s;
    return true;
  }
  return false;
}

inline auto ConsumeIfAny(const char*& s, const char* pred) -> bool {
  assert(pred != nullptr);
  const char* p = pred;
  while (*p) {
    if (*s == *p) {
      ++s;
      return true;
    }
    ++p;
  }
  return false;
}

}  // namespace RG

#endif  // REGGEN_COMMON_TEXT_H