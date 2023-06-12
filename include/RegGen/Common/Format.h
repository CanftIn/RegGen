#ifndef REGGEX_COMMON_FORMAT_H
#define REGGEX_COMMON_FORMAT_H

#include <stdexcept>
#include <string>

namespace RG {

class FormatError : public std::runtime_error {
 public:
  explicit FormatError(const std::string& msg) : std::runtime_error(msg) {}
};

template <typename... Args>
auto Format(const char* formatter, const Args&... args) -> std::string;

template <typename... Args>
auto Format(const std::string& formatter, const Args&... args) -> std::string;

template <typename... Args>
auto PrintFormatted(const char* formatter, const Args&... args) -> void;

template <typename... Args>
auto PrintFormatted(const std::string& formatter, const Args&... args) -> void;

namespace Internal {

inline void FormatAssert(bool pred, std::string msg = "unknown format error.") {
  if (!pred) {
    throw FormatError(msg);
  }
}

template <typename Stream, typename... Args>
auto FormatImpl(Stream& output, const std::string& formatter,
                const Args&... args) -> void;

}  // namespace Internal

}  // namespace RG

#endif  // REGGEX_COMMON_FORMAT_H