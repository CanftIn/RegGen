#ifndef REGGEN_PARSER_META_INFO_H
#define REGGEN_PARSER_META_INFO_H

#include <string>

#include "RegGen/AST/ASTHandle.h"
#include "RegGen/AST/ASTTypeProxy.h"
#include "RegGen/Common/Array.h"
#include "RegGen/Common/SmallVector.h"
#include "RegGen/Lexer/Regex.h"

namespace RG {

class TypeInfo;
class EnumTypeInfo;
class BaseTypeInfo;
class KlassTypeInfo;

class SymbolInfo;
class TokenInfo;
class VariableInfo;

class ProductionInfo;

class MetaInfo {
 public:
  class Builder;

  auto Environment() const -> const auto& { return env_; }

  // assumes last variable is root
  auto RootVariable() const -> const auto& { return variables_.back(); }

  auto Enums() const -> const auto& { return enums_; }
  auto Bases() const -> const auto& { return bases_; }
  auto Classes() const -> const auto& { return classes_; }

  auto Tokens() const -> const auto& { return tokens_; }
  auto IgnoredTokens() const -> const auto& { return ignored_tokens_; }
  auto Variables() const -> const auto& { return variables_; }
  auto Productions() const -> const auto& { return productions_; }

  auto LookupType(const std::string& name) const -> const auto& {
    return type_lookup_.at(name);
  }
  auto LookupSymbol(const std::string& name) const -> const auto& {
    return symbol_lookup_.at(name);
  }

  auto RootSymbol() const -> const auto& { return variables_.back(); }

 private:
  using TypeInfoMap = std::unordered_map<std::string, TypeInfo*>;
  using SymbolInfoMap = std::unordered_map<std::string, SymbolInfo*>;

  const AST::ASTTypeProxyManager* env_;

  TypeInfoMap type_lookup_;
  Array<EnumTypeInfo, 0> enums_;
  Array<BaseTypeInfo, 0> bases_;
  Array<KlassTypeInfo, 0> classes_;

  SymbolInfoMap symbol_lookup_;
  Array<TokenInfo, 0> tokens_;
  Array<TokenInfo, 0> ignored_tokens_;
  Array<VariableInfo, 0> variables_;
  Array<ProductionInfo, 0> productions_;
};

auto ResolveParserInfo(const std::string& config,
                       const AST::ASTTypeProxyManager* env)
    -> std::unique_ptr<MetaInfo>;
}  // namespace RG

#endif  // REGGEN_PARSER_META_INFO_H