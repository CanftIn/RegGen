#ifndef REGGEX_AST_AST_BASIC_H
#define REGGEX_AST_AST_BASIC_H

#include <type_traits>
#include <vector>

#include "RegGen/Common/Error.h"
#include "RegGen/Common/TypeTrait.h"
#include "RegGen/Container/SmallVector.h"

namespace RG::AST {

struct LocationInfo {
  int offset;
  int length;
};

class ASTNodeBase {
 public:
  explicit ASTNodeBase(int offset = -1, int length = -1)
      : offset_(offset), length_(length) {}

  auto Offset() const -> const auto& { return offset_; }
  auto Length() const -> const auto& { return length_; }

  auto GetLocationInfo() const -> LocationInfo {
    return LocationInfo{offset_, length_};
  }

  auto UpdateLocationInfo(const LocationInfo& info) -> void {
    offset_ = info.offset;
    length_ = info.length;
  }

 private:
  int offset_;
  int length_;
};

class BasicASTToken : public ASTNodeBase {
 public:
  BasicASTToken() = default;
  BasicASTToken(int offset, int length, int tag)
      : ASTNodeBase(offset, length), tag_(tag) {}

  auto Tag() const -> const auto& { return tag_; }
  auto IsValid() const -> bool { return tag_ != -1; }

 private:
  int tag_ = -1;
};

template <typename EnumType>
class BasicASTEnum : public ASTNodeBase {
  static_assert(Constraint<EnumType>(is_enum_of<int>));

 public:
  using UnderlyingType = EnumType;

  BasicASTEnum() = default;
  explicit BasicASTEnum(EnumType v) : value_(static_cast<int>(v)) {}

  auto IntValue() const -> int { return value_; }
  auto Value() const -> EnumType { return static_cast<EnumType>(value_); }
  auto IsValid() const -> bool { return value_ != -1; }

 private:
  int value_ = -1;
};

template <typename EnumType>
inline auto operator==(const BasicASTEnum<EnumType>& lhs,
                       const BasicASTEnum<EnumType>& rhs) -> bool {
  return lhs.Value() == rhs.Value();
}
template <typename EnumType>
inline auto operator==(const BasicASTEnum<EnumType>& lhs, const EnumType& rhs)
    -> bool {
  return lhs.Value() == rhs;
}
template <typename EnumType>
inline auto operator==(const EnumType& lhs, const BasicASTEnum<EnumType>& rhs)
    -> bool {
  return lhs == rhs.Value();
}

class BasicASTObject : public ASTNodeBase {
 public:
  BasicASTObject() = default;
  virtual ~BasicASTObject() = default;
};

template <typename T>
class ASTVector : public ASTNodeBase {
 public:
  using ElementType = T;

  auto Value() const -> const auto& { return container_; }

  auto Empty() const -> bool { return container_.empty(); }
  auto Size() const -> int { return container_.size(); }

  void PushBack(const T& value) { container_.push_back(value); }

 private:
  SmallVector<T> container_;
};

template <typename T>
class ASTOptional : public ASTNodeBase {
 public:
  using ElementType = T;

  ASTOptional() = default;

  explicit ASTOptional(const ElementType& value)
      : ASTNodeBase(value.Offset(), value.Length()), value_(value) {}

  auto HasValue() const -> bool { return value_.IsValid(); }

  auto Value() const -> const auto& {
    assert(HasValue());
    return value_;
  }

 private:
  ElementType value_ = {};
};

template <typename T>
class ASTOptional<T*> : public ASTNodeBase {
 public:
  using ElementType = T*;

  ASTOptional() = default;

  explicit ASTOptional(const ElementType& obj)
      : ASTNodeBase(obj->Offset(), obj->Length()), value_(obj) {}

  auto HasValue() const -> bool { return value_ != nullptr; }

  auto Value() const -> const auto& {
    assert(HasValue());
    return value_;
  }

 private:
  T* value_ = nullptr;
};

}  // namespace RG::AST

#endif  // REGGEX_AST_AST_BASIC_H