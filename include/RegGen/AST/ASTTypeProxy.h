#ifndef REGGEX_AST_AST_TYPE_PROXY_H
#define REGGEX_AST_AST_TYPE_PROXY_H

#include <type_traits>
#include <unordered_map>

#include "RegGen/AST/ASTBasic.h"
#include "RegGen/AST/ASTItem.h"
#include "RegGen/AST/ASTTypeTrait.h"
#include "RegGen/Common/Error.h"
#include "RegGen/Container/Arena.h"

namespace RG::AST {

class ASTTypeProxy {
 public:
  virtual auto ConstructEnum(int value) const -> ASTItem = 0;
  virtual auto ConstructObject(Arena&) const -> ASTItem = 0;
  virtual auto ConstructVector(Arena&) const -> ASTItem = 0;
  virtual auto ConstructOptional() const -> ASTItem = 0;

  virtual auto AssignField(ASTItem obj, int condition, ASTItem value) const
      -> void = 0;
  virtual auto PushBackElement(ASTItem vec, ASTItem elem) const -> void = 0;
};

class DummyASTTypeProxy : public ASTTypeProxy {
 private:
  [[noreturn]] static void Throw() {
    throw ParserInternalError{
        "DummyASTTypeProxy: Cannot perform any proxy operation."};
  }

 public:
  auto ConstructEnum(int /*value*/) const -> ASTItem override { Throw(); }
  auto ConstructObject(Arena&) const -> ASTItem override { Throw(); }
  auto ConstructVector(Arena&) const -> ASTItem override { Throw(); }
  auto ConstructOptional() const -> ASTItem override { Throw(); }

  auto AssignField(ASTItem /*obj*/, int /*condition*/, ASTItem /*value*/) const
      -> void override {
    Throw();
  }
  auto PushBackElement(ASTItem /*vec*/, ASTItem /*elem*/) const
      -> void override {
    Throw();
  }

  static auto Instance() -> const ASTTypeProxy& {
    static DummyASTTypeProxy dummy{};
    return dummy;
  }
};

template <typename T>
class BasicASTTypeProxy final : public ASTTypeProxy {
 public:
  using TraitType = ASTTypeTrait<T>;

  using SelfType = typename TraitType::SelfType;
  using StoreType = typename TraitType::StoreType;
  using VectorType = typename TraitType::VectorType;
  using OptionalType = typename TraitType::OptionalType;

  auto ConstructEnum(int value) const -> ASTItem override {
    if constexpr (TraitType::IsEnum()) {
      return SelfType{static_cast<typename SelfType::UnderlyingType>(value)};
    } else {
      throw ParserInternalError{"BasicASTTypeProxy: T is not an Enum type."};
    }
  }

  auto ConstructObject(Arena& arena) const -> ASTItem override {
    if constexpr (TraitType::IsClass()) {
      return arena.Construct<SelfType>();
    } else {
      throw ParserInternalError{"BasicASTTypeProxy: T is not a Class type."};
    }
  }

  auto ConstructVector(Arena& arena) const -> ASTItem override {
    return arena.Construct<VectorType>();
  }

  auto ConstructOptional() const -> ASTItem override { return OptionalType{}; }

  auto AssignField(ASTItem obj, int ordinal, ASTItem value) const
      -> void override {
    if constexpr (TraitType::IsClass()) {
      obj.Extract<SelfType*>()->SetItem(ordinal, value);
    } else {
      throw ParserInternalError{"BasicASTTypeProxy: T is not a Class type."};
    }
  }

  auto PushBackElement(ASTItem vec, ASTItem elem) const -> void override {
    vec.Extract<VectorType*>()->PushBack(elem.Extract<StoreType>());
  }
};

class ASTTypeProxyManager {
 public:
  auto Lookup(const std::string& name) const -> const ASTTypeProxy* {
    auto it = proxies_.find(name);
    if (it != proxies_.end()) {
      return it->second.get();
    } else {
      throw ParserInternalError{
          "ASTTypeProxyManager: Specific type proxy not found."};
    }
  }

  template <typename EnumType>
  auto RegisterEnum(const std::string& name) -> void {
    static_assert(Constraint<EnumType>(is_enum_of<int>));

    RegisterType<BasicASTEnum<EnumType>>(name);
  }

  template <typename ClassType>
  auto RegisterClass(const std::string& name) -> void {
    static_assert(Constraint<ClassType>(derive_from<BasicASTObject>));

    RegisterType<ClassType>(name);
  }

 private:
  template <typename ASTType>
  auto RegisterType(const std::string& name) -> void {
    proxies_.emplace(name, std::make_unique<BasicASTTypeProxy<ASTType>>());
  }

  using ProxyLookup =
      std::unordered_map<std::string, std::unique_ptr<ASTTypeProxy>>;
  ProxyLookup proxies_;
};

}  // namespace RG::AST

#endif  // REGGEX_AST_AST_TYPE_PROXY_H