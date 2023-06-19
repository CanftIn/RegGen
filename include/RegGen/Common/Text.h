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

class CharRange {
 public:
  CharRange(int min, int max) : min_(min), max_(max) { assert(min <= max); }
  explicit CharRange(int ch) : CharRange(ch, ch) {}

  auto Min() const -> int { return min_; }
  auto Max() const -> int { return max_; }

  auto Length() const -> int { return max_ - min_ + 1; }

  auto Contain(int ch) const -> bool { return ch >= min_ && ch <= max_; }
  auto Contain(CharRange rg) const -> bool {
    return rg.min_ >= min_ && rg.max_ <= max_;
  }

 private:
  int min_, max_;
};

inline auto EscapeRawCharacter(int ch) -> int {
  switch (ch) {
    case 'a':
      return '\a';
    case 'b':
      return '\b';
    case 't':
      return '\t';
    case 'r':
      return '\r';
    case 'v':
      return '\v';
    case 'f':
      return '\f';
    case 'n':
      return '\n';
    default:
      return ch;
  }
}

inline auto IsEof(const char* s) -> bool { return *s == '\0'; }

inline auto SkipWhitespace(const char*& s, bool toggle = true) -> void {
  if (!toggle) {
    return;
  }

  bool halt = false;
  while (!halt) {
    halt = true;

    if (*s == '#') {
      while (*s && *s != '\n') {
        Consume(s);
      }

      halt = false;
    }

    while (ConsumeIfAny(s, " \r\n\t")) {
      halt = false;
    }
  }
}

}  // namespace RG

#endif  // REGGEN_COMMON_TEXT_H