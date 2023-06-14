#ifndef REGGEX_AST_AST_TYPE_TRAITS_H
#define REGGEX_AST_AST_TYPE_TRAITS_H

#include <type_traits>

#include "RegGen/AST/ASTBasic.h"
#include "RegGen/Common/Type.h"

namespace RG::AST {

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
      static_assert(std::false_type::value, "Invalid AST type.");
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

#endif  // REGGEX_AST_AST_TYPE_TRAITS_H