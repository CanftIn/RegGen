#ifndef REGGEX_LEXER_REGEX_H
#define REGGEX_LEXER_REGEX_H

#include <cassert>
#include <functional>
#include <memory>
#include <vector>

#include "RegGen/Common/Text.h"
#include "RegGen/Container/SmallVector.h"

namespace RG {

class RegexExpr;
class LabelExpr;

class RootExpr;
class EntityExpr;
class SequenceExpr;
class ChoiceExpr;
class ClosureExpr;

using RegexExprPtr = std::unique_ptr<RegexExpr>;
using RegexExprVec = SmallVector<RegexExprPtr, 0>;

auto ParseRegex(const std::string& regex) -> std::unique_ptr<RootExpr>;

// auto ParseRegexInternal(const char*& str, char term) -> RegexExprPtr;
// auto ParseCharClass(const char*& str) -> RegexExprPtr;
// auto ParseEscapedExpr(const char*& str) -> RegexExprPtr;
// auto ParseCharacter(const char*& str) -> int;

enum class RepetitionMode : uint8_t {
  Optional, // * or +
  Star,
  Plus,
};

class RegexExprVisitor {
 public:
  virtual auto Visit(const RootExpr&) -> void = 0;
  virtual auto Visit(const EntityExpr&) -> void = 0;
  virtual auto Visit(const SequenceExpr&) -> void = 0;
  virtual auto Visit(const ChoiceExpr&) -> void = 0;
  virtual auto Visit(const ClosureExpr&) -> void = 0;
};

class RegexExpr {
 public:
  RegexExpr() = default;
  virtual ~RegexExpr() = default;

  virtual auto Accept(RegexExprVisitor& visitor) const -> void = 0;
};

class LabelExpr {
 public:
  LabelExpr() = default;
  virtual ~LabelExpr() = default;

  virtual auto TestPassage(int c) const -> bool = 0;
};

class RootExpr : public RegexExpr, public LabelExpr {
 public:
  RootExpr(RegexExprPtr expr) : expr_(std::move(expr)) {}

  auto Child() const -> const auto& { return expr_; }

  auto Accept(RegexExprVisitor& visitor) const -> void override {
    visitor.Visit(*this);
  }

  auto TestPassage(int /*c*/) const -> bool override { return false; }

 private:
  RegexExprPtr expr_;
};

class EntityExpr : public RegexExpr, public LabelExpr {
 public:
  EntityExpr(CharRange cg) : range_(cg) {}

  auto Range() const -> const auto& { return range_; }

  auto Accept(RegexExprVisitor& visitor) const -> void override {
    visitor.Visit(*this);
  }

  auto TestPassage(int c) const -> bool override { return range_.Contain(c); }

 private:
  CharRange range_;
};

class SequenceExpr : public RegexExpr {
 public:
  SequenceExpr(RegexExprVec exprs) : exprs_(std::move(exprs)) {
    assert(!exprs_.empty());
  }

  auto Child() const -> const auto& { return exprs_; }

  auto Accept(RegexExprVisitor& visitor) const -> void override {
    visitor.Visit(*this);
  }

 private:
  RegexExprVec exprs_;
};

class ChoiceExpr : public RegexExpr {
 public:
  ChoiceExpr(RegexExprVec exprs) : exprs_(std::move(exprs)) {
    assert(!exprs_.empty());
  }

  auto Child() const -> const auto& { return exprs_; }

  auto Accept(RegexExprVisitor& visitor) const -> void override {
    visitor.Visit(*this);
  }

 private:
  RegexExprVec exprs_;
};

class ClosureExpr : public RegexExpr {
 public:
  ClosureExpr(RegexExprPtr expr, RepetitionMode mode)
      : expr_(std::move(expr)), mode_(mode) {}

  auto Child() const -> const auto& { return expr_; }
  auto Mode() const -> RepetitionMode { return mode_; }

  void Accept(RegexExprVisitor& visitor) const override {
    visitor.Visit(*this);
  }

 private:
  RegexExprPtr expr_;
  RepetitionMode mode_;
};

}  // namespace RG

#endif  // REGGEX_LEXER_REGEX_H