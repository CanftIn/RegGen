#ifndef REGGEX_LEXER_REGEX_H
#define REGGEX_LEXER_REGEX_H

#include <cassert>
#include <functional>
#include <memory>
#include <vector>

#include "RegGen/Common/Text.h"

namespace RG {

class RegexExpr;
class LabelExpr;

class RootExpr;
class EntityExpr;
class SequenceExpr;
class ChoiceExpr;
class ClosureExpr;

using RegexExprPtr = std::unique_ptr<RegexExpr>;
using RegexExprVec = std::vector<RegexExprPtr>;

auto ParseRegex(const std::string& regex) -> std::unique_ptr<RootExpr>;

enum class RepetitionMode : uint8_t {
  Optional,
  Star,
  Plus,
};

class RegexExprVisitor {
 public:
  virtual void Visit(const RootExpr&) = 0;
  virtual void Visit(const EntityExpr&) = 0;
  virtual void Visit(const SequenceExpr&) = 0;
  virtual void Visit(const ChoiceExpr&) = 0;
  virtual void Visit(const ClosureExpr&) = 0;
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

class RootExpr : public RegexExpr, LabelExpr {
 public:
  explicit RootExpr(RegexExprPtr expr) : expr_(std::move(expr)) {}

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
  explicit EntityExpr(CharRange cg) : range_(cg) {}

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
  SequenceExpr(std::vector<RegexExprPtr> exprs) : exprs_(std::move(exprs)) {
    assert(!exprs.empty());
  }

  auto Child() const -> const auto& { return exprs_; }

  auto Accept(RegexExprVisitor& visitor) const -> void override {
    visitor.Visit(*this);
  }

 private:
  std::vector<RegexExprPtr> exprs_;
};

class ChoiceExpr : public RegexExpr {
 public:
  ChoiceExpr(std::vector<RegexExprPtr> exprs) : exprs_(std::move(exprs)) {
    assert(!exprs.empty());
  }

  auto Child() const -> const auto& { return exprs_; }

  auto Accept(RegexExprVisitor& visitor) const -> void override {
    visitor.Visit(*this);
  }

 private:
  std::vector<RegexExprPtr> exprs_;
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