#ifndef REGGEX_AST_AST_ITEM_H
#define REGGEX_AST_AST_ITEM_H

#include <cstddef>
#include <typeindex>

#include "RegGen/AST/ASTBasic.h"
#include "RegGen/AST/ASTTypeTrait.h"
#include "RegGen/Common/TypeTrait.h"

namespace RG::AST {

class ASTItem {
 public:
  ASTItem() = default;

  template <typename T>
  ASTItem(const T& value) {
    Assign(value);
  }

  auto HasValue() -> bool { return type_ != nullptr; }

  auto Clear() -> void { type_ = nullptr; }

  template <typename T>
  auto DetectInstance() -> bool {
    AssertASTItem<T>();
    return type_ == GetTypeMetaInfo<T>();
  }

  template <typename T>
  auto Extract() -> T {
    if constexpr (Constraint<T>(Internal::is_astitem_object)) {
      auto result = dynamic_cast<T>(RefAs<BasicASTObject*, false>());

      if (result == nullptr) {
        ThrowTypeMismatch();
      }
      return result;
    } else if constexpr (Constraint<T>(Internal::is_astitem_optional)) {
      if (DetectInstance<T>()) {
        return RefAs<T, false>();
      } else {
        return Extract<typename T::ElementType>();
      }
    } else {
      return RefAs<T, false>();
    }
  }

  template <typename T>
  auto Assign(T value) -> void {
    if constexpr (Constraint<T>(convertible_to<BasicASTObject*> &&
                                !same_to<std::nullptr_t>)) {
      assert(value != nullptr);
      RefAs<BasicASTObject*, true>() = value;
    } else {
      RefAs<T, true>() = value;
    }
  }

  auto GetLocationInfo() -> LocationInfo {
    return RefNodeBase().GetLocationInfo();
  }

  auto UpdateLocationInfo(int offset, int length) -> void {
    RefNodeBase().UpdateLocationInfo({offset, length});
  }

 private:
  template <typename T>
  void AssertASTItem() {
    static_assert(Internal::IsASTItem<T>(), "T must be a valid AstItem");
    static_assert(Constraint<T>(!is_const && !is_volatile),
                  "T should not be cv-qualified");
    static_assert(Constraint<T>(!is_reference), "T should not be reference");
  }

  template <typename T, bool ForceType>
  auto RefAs() -> T& {
    if constexpr (ForceType) {
      type_ = GetTypeMetaInfo<T>();
    } else if (!DetectInstance<T>()) {
      ThrowTypeMismatch();
    }

    return *reinterpret_cast<T*>(&data_);
  }

  auto RefNodeBase() -> ASTNodeBase& {
    if (!HasValue()) {
      ThrowTypeMismatch();
    }

    if (type_->store_in_heap) {
      if (type_->is_object) {
        return **reinterpret_cast<BasicASTObject**>(&data_);
      } else {
        return **reinterpret_cast<ASTNodeBase**>(&data_);
      }
    } else {
      return *reinterpret_cast<ASTNodeBase*>(&data_);
    }
  }

  [[noreturn]] void ThrowTypeMismatch() {
    throw ParserInternalError{"ASTItem: Storage type mismatch."};
  }

  struct TypeMetaInfo {
    std::type_index type_info;
    bool store_in_heap;
    bool is_object;
  };

  template <typename T>
  static auto GetTypeMetaInfo() {
    static const TypeMetaInfo instance = {
        typeid(T), std::is_pointer_v<T>,
        Constraint<T>(convertible_to<BasicASTObject*> &&
                      !same_to<std::nullptr_t>)};
    return &instance;
  }

  using StorageType =
      std::aligned_union_t<4, BasicASTToken, BasicASTEnum<ASTTypeCategory>,
                           std::nullptr_t>;

  const TypeMetaInfo* type_ = nullptr;
  StorageType data_ = {};
};

}  // namespace RG::AST

#endif  // REGGEX_AST_AST_ITEM_H