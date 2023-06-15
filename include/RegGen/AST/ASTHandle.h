#ifndef REGGEN_AST_AST_HANDLE_H
#define REGGEN_AST_AST_HANDLE_H

#include <string_view>
#include <variant>

#include "RegGen/AST/ASTBasic.h"
#include "RegGen/AST/ASTItem.h"
#include "RegGen/AST/ASTTypeProxy.h"
#include "RegGen/AST/ASTTypeTrait.h"
#include "RegGen/Container/ArrayRef.h"
#include "RegGen/Container/SmallVector.h"

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

class ASTManipPlaceholder {
 public:
  void Invoke(const ASTTypeProxy& proxy, ASTItem item,
              ArrayRef<ASTItem> rhs) const {}
};

class ASTObjectSetter {
 public:
  struct SetterPair {
    int member_index;
    int symbol_index;
  };

  explicit ASTObjectSetter(const SmallVector<SetterPair>& setters)
      : setters_(setters) {}

  void Invoke(const ASTTypeProxy& proxy, ASTItem obj,
              ArrayRef<ASTItem> rhs) const {
    for (auto setter : setters_) {
      proxy.AssignField(obj, setter.member_index, rhs[setter.symbol_index]);
    }
  }

 private:
  SmallVector<SetterPair> setters_;
};

class ASTVectorMerger {
 public:
  explicit ASTVectorMerger(const SmallVector<int>& indices)
      : indices_(indices) {}

  void Invoke(const ASTTypeProxy& proxy, ASTItem vec,
              ArrayRef<ASTItem> rhs) const {
    for (auto index : indices_) {
      proxy.PushBackElement(vec, rhs[index]);
    }
  }

 private:
  SmallVector<int> indices_;
};

class ASTHandle {
 public:
  using GenHandle = std::variant<ASTEnumGen, ASTObjectGen, ASTVectorGen,
                                 ASTOptionalGen, ASTItemSelector>;

  using ManipHandle =
      std::variant<ASTManipPlaceholder, ASTObjectSetter, ASTVectorMerger>;

  ASTHandle(const ASTTypeProxy* proxy, GenHandle gen, ManipHandle manip)
      : proxy_(proxy), gen_handle_(gen), manip_handle_(manip) {}

  auto Invoke(Arena& arena, ArrayRef<ASTItem> rhs) const -> ASTItem {
    auto gen_visitor = [&](const auto& gen) {
      return gen.Invoke(*proxy_, arena, rhs);
    };
    auto result = std::visit(gen_visitor, gen_handle_);

    auto manip_visitor = [&](const auto& manip) {
      manip.Invoke(*proxy_, result, rhs);
    };
    std::visit(manip_visitor, manip_handle_);

    auto front_loc = const_cast<ASTItem&>(rhs.front()).GetLocationInfo();
    auto back_loc = const_cast<ASTItem&>(rhs.back()).GetLocationInfo();

    auto offset = front_loc.offset;
    auto length = back_loc.offset + back_loc.length - offset;
    result.UpdateLocationInfo(offset, length);

    return result;
  }

 private:
  const ASTTypeProxy* proxy_;

  GenHandle gen_handle_;
  ManipHandle manip_handle_;
};

}  // namespace RG::AST

#endif  // REGGEN_AST_AST_HANDLE_H