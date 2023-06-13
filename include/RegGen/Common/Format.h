#ifndef REGGEX_COMMON_FORMAT_H
#define REGGEX_COMMON_FORMAT_H

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "RegGen/Common/Error.h"
#include "RegGen/Common/Text.h"

namespace RG {

template <typename... Args>
auto Format(const char* formatter, const Args&... args) -> std::string;

template <typename... Args>
auto Format(const std::string& formatter, const Args&... args) -> std::string;

template <typename... Args>
auto PrintFormatted(const char* formatter, const Args&... args) -> void;

template <typename... Args>
auto PrintFormatted(const std::string& formatter, const Args&... args) -> void;

namespace Internal {

inline void FormatAssert(bool pred, std::string msg = "Unknown format error.") {
  if (!pred) {
    throw FormatError(msg);
  }
}

template <typename Stream, typename... Args>
auto FormatImpl(Stream& output, const std::string& formatter,
                const Args&... args) -> void {
  static_assert(sizeof...(args) < 11, "Only support 10 args.");

  if (formatter.empty()) {
    return;
  }

  constexpr auto ArgsCount = sizeof...(args);
  std::function<void(Stream&)> helpers[10] = {[&](auto& ss) { ss << args; }...};

  size_t next_id = 0;
  for (const auto* p = formatter.c_str(); *p;) {
    if (ConsumeIf(p, '{')) {
      if (ConsumeIf(p, '{')) {
        output.put('{');
      } else {
        size_t id;
        if (ConsumeIf(p, '}')) {
          id = next_id++;
        } else {
          assert(IsDigit(*p));
          id = Consume(p) - '0';
          FormatAssert(id >= 0 && id <= 9,
                       "Argument id must be within [0,10).");
          FormatAssert(id < ArgsCount, "Not enough arguments.");

          FormatAssert(ConsumeIf(p, '}'), "Invalid argument reference.");
        }
        helpers[id](output);
      }
    } else if (ConsumeIf(p, '}')) {
      if (ConsumeIf(p, '}')) {
        output.put('}');
      } else {
        FormatAssert(false, "An isolated closing brace is not allowed.");
      }
    } else {
      output.put(Consume(p));
    }
  }
}

}  // namespace Internal

template <typename... Args>
auto Format(const char* formatter, const Args&... args) -> std::string {
  std::stringstream ss;
  Internal::FormatImpl(ss, formatter, args...);
  return ss.str();
}

template <typename... Args>
auto Format(const std::string& formatter, const Args&... args) -> std::string {
  return Format(formatter.c_str(), args...);
}

template <typename... Args>
auto PrintFormatted(const char* formatter, const Args&... args) -> void {
  Internal::FormatImpl(std::cout, formatter, args...);
}

template <typename... Args>
auto PrintFormatted(const std::string& formatter, const Args&... args) -> void {
  PrintFormatted(formatter.c_str(), args...);
}

}  // namespace RG

#endif  // REGGEX_COMMON_FORMAT_H