#ifndef REGGEX_COMMON_ERROR_H
#define REGGEX_COMMON_ERROR_H

#include <stdexcept>
#include <string>

namespace RG {

class FormatError : public std::runtime_error {
 public:
  explicit FormatError(const std::string& msg) : std::runtime_error(msg) {}
};

class ParserConstructionError : std::runtime_error {
 public:
  explicit ParserConstructionError(const std::string& msg)
      : std::runtime_error(msg) {}
};

class ParserInternalError : std::runtime_error {
 public:
  explicit ParserInternalError(const std::string& msg)
      : std::runtime_error(msg) {}
};

}  // namespace RG

#endif  // REGGEX_COMMON_ERROR_H