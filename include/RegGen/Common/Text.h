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
  CharRange(int begin, int end) : begin_(begin), end_(end) {}

  explicit CharRange(int c) : CharRange(c, c) {}

  auto Begin() const -> int { return begin_; }
  auto End() const -> int { return end_; }

  auto Length() const -> int { return end_ - begin_ + 1; }

  auto Contain(int c) const -> bool { return begin_ <= c && c <= end_; }
  auto Contain(CharRange cg) const -> bool {
    return begin_ <= cg.begin_ && cg.end_ <= end_;
  }

 private:
  int begin_;
  int end_;
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

}  // namespace RG

#endif  // REGGEN_COMMON_TEXT_H