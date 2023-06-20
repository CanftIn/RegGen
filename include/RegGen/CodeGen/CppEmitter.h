#ifndef REGGEN_CODEGEN_CPP_EMITTER_H
#define REGGEN_CODEGEN_CPP_EMITTER_H

#include <functional>
#include <sstream>
#include <string>

#include "RegGen/Common/Format.h"

namespace RG {

class CppEmitter {
 public:
  using CallbackType = std::function<void()>;

  auto EmptyLine() -> void;
  auto Comment(std::string_view s) -> void;
  auto Include(std::string_view s, bool system) -> void;
  auto Namespace(std::string_view name, CallbackType cb) -> void;
  auto Class(std::string_view name, std::string_view parent, CallbackType cb)
      -> void;
  auto Struct(std::string_view name, std::string_view parent, CallbackType cb)
      -> void;
  auto Enum(std::string_view name, std::string_view type, CallbackType cb)
      -> void;
  auto Block(std::string_view header, CallbackType cb) -> void;
  auto ToString() -> std::string;

  template <typename... Args>
  auto WriteLine(const char* fmt, const Args&... args) -> void {
    WriteIndent();
    buffer_ << Format(fmt, args...) << std::endl;
  }

 private:
  auto WriteIndent() -> void;
  auto WriteBlock(std::string_view header, bool semi, CallbackType cb) -> void;
  auto WriteStructure(std::string_view type, std::string_view name,
                      std::string_view parent, bool semi, CallbackType cb)
      -> void;

  int indent_level_ = 0;
  std::ostringstream buffer_;
};

}  // namespace RG

#endif  // REGGEN_CODEGEN_CPP_EMITTER_H