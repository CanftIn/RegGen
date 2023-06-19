#ifndef REGGEN_AST_DATA_BUNDLE_H
#define REGGEN_AST_DATA_BUNDLE_H

#include <cassert>

#include "RegGen/AST/ASTBasic.h"
#include "RegGen/AST/ASTItem.h"

namespace RG::AST {

template <typename... Ts>
class EmptyClass;

template <typename... Ts>
class DataBundle {
 public:
  void SetItem(int ordinal, ASTItem data) { throw 0; }

  template <int Ordinal>
  auto GetItem() -> const auto& {
    static_assert(false);
    return BasicASTObject();
  }
};

template <typename T, typename... Ts>
class DataBundle<T, Ts...> {
 public:
  void SetItem(int ordinal, ASTItem data) {
    if (ordinal == 0) {
      first_ = data.Extract<T>();
    } else {
      rest_.SetItem(ordinal - 1, data);
    }
  }

  template <int Ordinal>
  auto GetItem() const -> const auto& {
    if constexpr (Ordinal == 0) {
      return first_;
    } else {
      static_assert(sizeof...(Ts) > 0);
      return rest_.template GetItem<(Ordinal - 1)>();
    }
  }

 private:
  T first_ = {};
  DataBundle<Ts...> rest_;
};

}  // namespace RG::AST

#endif  // REGGEN_AST_DATA_BUNDLE_H