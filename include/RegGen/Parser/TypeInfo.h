#ifndef REGGEN_PARSER_TYPE_INFO_H
#define REGGEN_PARSER_TYPE_INFO_H

#include <string>
#include <utility>

#include "RegGen/Common/SmallVector.h"
#include "RegGen/Parser/MetaInfo.h"

namespace RG {

class TypeInfo {
 public:
  enum class Category {
    Token,
    Enum,
    Base,
    Class,
  };

  explicit TypeInfo(std::string name) : name_(std::move(name)) {}

  auto Name() const -> const auto& { return name_; }

  virtual auto GetCategory() const -> Category = 0;

  auto IsToken() const -> bool { return GetCategory() == Category::Token; }
  auto IsEnum() const -> bool { return GetCategory() == Category::Enum; }
  auto IsBase() const -> bool { return GetCategory() == Category::Base; }
  auto IsClass() const -> bool { return GetCategory() == Category::Class; }

  auto IsStoredByRef() const -> bool {
    auto category = GetCategory();
    return category == Category::Base || category == Category::Class;
  }

 private:
  std::string name_;
};

struct TypeSpec {
  enum class Qualifier { None, Vector, Optional };

  Qualifier qual;
  std::unique_ptr<TypeInfo> type;

 public:
  auto IsNoneQualified() const -> bool { return qual == Qualifier::None; }
  auto IsVector() const -> bool { return qual == Qualifier::Vector; }
  auto IsOptional() const -> bool { return qual == Qualifier::Optional; }
};

struct TokenTypeInfo : public TypeInfo {
 private:
  TokenTypeInfo() : TypeInfo("token") {}

 public:
  auto GetCategory() const -> Category override { return Category::Token; }

  static auto Instance() -> TokenTypeInfo& {
    static TokenTypeInfo dummy{};

    return dummy;
  }
};

class EnumTypeInfo : public TypeInfo {
 public:
  explicit EnumTypeInfo(const std::string& name = "") : TypeInfo(name) {}

  auto GetCategory() const -> Category override { return Category::Enum; }

  auto Values() const -> const auto& { return values_; }

 private:
  friend class MetaInfo::Builder;

  SmallVector<std::string> values_;
};

class BaseTypeInfo : public TypeInfo {
 public:
  explicit BaseTypeInfo(const std::string& name = "") : TypeInfo(name) {}

  auto GetCategory() const -> Category override { return Category::Base; }
};

class ClassTypeInfo : public TypeInfo {
 public:
  struct MemberInfo {
    TypeSpec type;
    std::string name;
  };

  explicit ClassTypeInfo(const std::string& name = "") : TypeInfo(name) {}

  auto GetCategory() const -> Category override { return Category::Class; }

  auto BaseType() const -> const auto& { return base_; }
  auto Members() const -> const auto& { return members_; }

 private:
  friend class MetaInfo::Builder;

  std::unique_ptr<BaseTypeInfo> base_ = nullptr;
  SmallVector<MemberInfo> members_;
};

class SymbolInfo {
 public:
  enum class Category { Token, Variable };

  SymbolInfo(int id, std::string name) : id_(id), name_(std::move(name)) {}

  auto Id() const -> const auto& { return id_; }
  auto Name() const -> const auto& { return name_; }

  virtual auto GetCategory() const -> Category = 0;

  auto AsToken() const {
    return IsToken() ? reinterpret_cast<const TokenInfo*>(this) : nullptr;
  }
  auto AsVariable() const {
    return IsVariable() ? reinterpret_cast<const VariableInfo*>(this) : nullptr;
  }

  auto IsToken() const -> bool { return GetCategory() == Category::Token; }
  auto IsVariable() const -> bool {
    return GetCategory() == Category::Variable;
  }

 private:
  int id_;
  std::string name_;
};

class VariableInfo : public SymbolInfo {
 public:
  explicit VariableInfo(int id = -1, const std::string& name = "")
      : SymbolInfo(id, name) {}

  auto GetCategory() const -> Category override { return Category::Variable; }

  auto Type() const -> const auto& { return type_; }
  auto Productions() const -> const auto& { return productions_; }

 private:
  friend class MetaInfo::Builder;

  TypeSpec type_;
  SmallVector<std::unique_ptr<ProductionInfo>> productions_;
};

class ProductionInfo {
 public:
  auto Left() const -> const auto& { return lhs_; }
  auto Right() const -> const auto& { return rhs_; }

  auto Handle() const -> const auto& { return handle_; }

 private:
  friend class MetaInfo::Builder;

  std::unique_ptr<VariableInfo> lhs_;
  SmallVector<std::unique_ptr<SymbolInfo>> rhs_;

  SmallVector<AST::ASTHandle> handle_;
};

}  // namespace RG

#endif  // REGGEN_PARSER_TYPE_INFO_H