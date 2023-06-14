#ifndef REGGEN_AST_AST_HANDLE_H
#define REGGEN_AST_AST_HANDLE_H

#include <string_view>
#include <variant>

#include "RegGen/AST/ASTBasic.h"
#include "RegGen/AST/ASTItem.h"
#include "RegGen/AST/ASTTypeProxy.h"
#include "RegGen/AST/ASTTypeTrait.h"
#include "RegGen/Common/ArrayRef.h"
#include "RegGen/Common/SmallVector.h"

namespace RG::AST {

class ASTEnumGen {
 public:
  explicit ASTEnumGen(int value) : value_(value) {}

  auto Invoke(const ASTTypeProxy& proxy, Arena& /*arena*/,
              ArrayRef<ASTItem> /*rhs*/) const -> ASTItem {
    return proxy.ConstructEnum(value_);
  }

 private:
  int value_;
};

class ASTObjectGen {
 public:
  auto Invoke(const ASTTypeProxy& proxy, Arena& arena,
              ArrayRef<ASTItem> /*rhs*/) const -> ASTItem {
    return proxy.ConstructObject(arena);
  }
};

class ASTVectorGen {
 public:
  auto Invoke(const ASTTypeProxy& proxy, Arena& arena,
              ArrayRef<ASTItem> /*rhs*/) const -> ASTItem {
    return proxy.ConstructVector(arena);
  }
};

class ASTOptionalGen {
 public:
  auto Invoke(const ASTTypeProxy& proxy, Arena& /*arena*/,
              ArrayRef<ASTItem> /*rhs*/) const -> ASTItem {
    return proxy.ConstructOptional();
  }
};

class ASTItemSelector {
 public:
  explicit ASTItemSelector(int index) : index_(index) {}

  auto Invoke(const ASTTypeProxy& /*proxy*/, Arena& /*arena*/,
              ArrayRef<ASTItem> rhs) const -> ASTItem {
    assert(index_ < rhs.size());
    return rhs[index_];
  }

 private:
  int index_;
};


}  // namespace RG::AST

#endif  // REGGEN_AST_AST_HANDLE_H