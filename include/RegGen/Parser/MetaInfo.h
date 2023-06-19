#ifndef REGGEN_PARSER_META_INFO_H
#define REGGEN_PARSER_META_INFO_H

#include <string>

#include "RegGen/AST/ASTHandle.h"
#include "RegGen/AST/ASTTypeProxy.h"
#include "RegGen/Container/HeapArray.h"
#include "RegGen/Container/SmallVector.h"
#include "RegGen/Lexer/Regex.h"

namespace RG {

class TypeInfo;
class EnumTypeInfo;
class BaseTypeInfo;
class ClassTypeInfo;

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
  using TypeInfoMap =
      std::unordered_map<std::string, TypeInfo*>;
  using SymbolInfoMap =
      std::unordered_map<std::string, SymbolInfo*>;

  const AST::ASTTypeProxyManager* env_;

  TypeInfoMap type_lookup_;
  HeapArray<EnumTypeInfo> enums_;
  HeapArray<BaseTypeInfo> bases_;
  HeapArray<ClassTypeInfo> classes_;

  SymbolInfoMap symbol_lookup_;
  HeapArray<TokenInfo> tokens_;
  HeapArray<TokenInfo> ignored_tokens_;
  HeapArray<VariableInfo> variables_;
  HeapArray<ProductionInfo> productions_;
};

auto ResolveParserInfo(const std::string& config,
                       const AST::ASTTypeProxyManager* env)
    -> std::unique_ptr<MetaInfo>;
}  // namespace RG

#endif  // REGGEN_PARSER_META_INFO_H