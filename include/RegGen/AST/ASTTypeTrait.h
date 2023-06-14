#ifndef REGGEX_AST_AST_TYPE_TRAIT_H
#define REGGEX_AST_AST_TYPE_TRAIT_H

#include <type_traits>

#include "RegGen/AST/ASTBasic.h"
#include "RegGen/Common/Type.h"

namespace RG::AST {

namespace Internal {

template <typename T>
struct IsBasicASTEnum : std::false_type {};

template <typename T>
struct IsBasicASTEnum<BasicASTEnum<T>> : std::true_type {};

static constexpr auto is_ast_token = same_to<BasicASTToken>;
static constexpr auto is_ast_enum = generic_type_checker<IsBasicASTEnum>;
static constexpr auto is_ast_object = derive_from<BasicASTObject>;

template <typename T>
struct IsASTVectorPtr : std::false_type {};

template <typename T>
struct IsASTVectorPtr<ASTVector<T>*> : std::true_type {};

template <typename T>
struct IsASTOptional : std::false_type {};

template <typename T>
struct IsASTOptional<ASTOptional<T>> : std::true_type {};

static constexpr auto is_astitem_token = is_ast_token;
static constexpr auto is_astitem_enum = is_ast_enum;
static constexpr auto is_astitem_object =
    convertible_to<BasicASTObject*> && !same_to<std::nullptr_t>;
static constexpr auto is_astitem_vector = generic_type_checker<IsASTVectorPtr>;
static constexpr auto is_astitem_optional = generic_type_checker<IsASTOptional>;

template <typename T>
inline constexpr auto IsASTItem() -> bool {
  return Constraint<T>(is_astitem_token || is_astitem_enum ||
                       is_astitem_object || is_astitem_vector ||
                       is_astitem_optional) &&
         std::is_trivially_destructible_v<T>;
}

}  // namespace Internal

enum class ASTTypeCategory {
  Token,
  Enum,
  Base,
  Class,
};

template <typename T>
struct ASTTypeTrait {
  static constexpr auto GetCategory() -> ASTTypeCategory {
    if constexpr (Constraint<T>(Internal::is_ast_token)) {
      return ASTTypeCategory::Token;
    } else if constexpr (Constraint<T>(Internal::is_ast_enum)) {
      return ASTTypeCategory::Enum;
    } else if constexpr (Constraint<T>(Internal::is_ast_object)) {
      if constexpr (Constraint<T>(is_abstract)) {
        return ASTTypeCategory::Base;
      } else {
        return ASTTypeCategory::Class;
      }
    } else {
      static_assert(AlwaysFalse<T>::value, "Invalid AST type.");
    }
  }

  static constexpr auto IsToken() -> bool {
    return GetCategory() == ASTTypeCategory::Token;
  }
  static constexpr auto IsEnum() -> bool {
    return GetCategory() == ASTTypeCategory::Enum;
  }
  static constexpr auto IsBase() -> bool {
    return GetCategory() == ASTTypeCategory::Base;
  }
  static constexpr auto IsClass() -> bool {
    return GetCategory() == ASTTypeCategory::Class;
  }

  static constexpr auto StoreInHeap() -> bool { return IsBase() || IsClass(); }

  static constexpr bool store_in_heap_value = StoreInHeap();
  using SelfType = T;
  using StorageType = std::conditional_t<store_in_heap_value, T*, T>;

  using VectorType = ASTVector<StorageType>;
  using OptionalType = ASTOptional<StorageType>;
};

}  // namespace RG::AST

#endif  // REGGEX_AST_AST_TYPE_TRAIT_H