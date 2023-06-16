#include "RegGen/CodeGen/CppEmitter.h"

#include "RegGen/Common/Format.h"

namespace RG {

auto CppEmitter::EmptyLine() -> void { buffer_ << "\n"; }

auto CppEmitter::Comment(std::string_view s) -> void {
  WriteIndent();
  buffer_ << "// " << s << "\n";
}

auto CppEmitter::Include(std::string_view s, bool system) -> void {
  WriteIndent();
  buffer_ << "#include " << (system ? "<" : "\"") << s << (system ? ">" : "\"")
          << "\n";
}

auto CppEmitter::Namespace(std::string_view name, CallbackType cb) -> void {
  WriteStructure("namespace", name, "", false, cb);
}

auto CppEmitter::CppEmitter::Class(std::string_view name,
                                   std::string_view parent, CallbackType cb)
    -> void {
  WriteStructure("class", name, parent, true, cb);
}

auto CppEmitter::Struct(std::string_view name, std::string_view parent,
                        CallbackType cb) -> void {
  WriteStructure("struct", name, parent, true, cb);
}

auto CppEmitter::Enum(std::string_view name, std::string_view type,
                      CallbackType cb) -> void {
  WriteStructure("enum", name, type, true, cb);
}

auto CppEmitter::Block(std::string_view header, CallbackType cb) -> void {
  WriteBlock(header, false, cb);
}

auto CppEmitter::ToString() -> std::string { return buffer_.str(); }

auto CppEmitter::WriteIndent() -> void {
  for (int i = 0; i < indent_level_; ++i) {
    buffer_ << "  ";
  }
}

auto CppEmitter::WriteBlock(std::string_view header, bool semi, CallbackType cb)
    -> void {
  WriteIndent();
  buffer_ << header << " {\n";
  ++indent_level_;
  cb();
  --indent_level_;
  WriteIndent();
  buffer_ << "}" << (semi ? ";" : "") << "\n";
}

auto CppEmitter::WriteStructure(std::string_view type, std::string_view name,
                                std::string_view parent, bool semi,
                                CallbackType cb) -> void {
  auto header = Format("{} {}", type, name);
  if (!parent.empty()) {
    header.append(" : ").append(parent);
  }

  WriteBlock(header, semi, cb);
}

}  // namespace RG