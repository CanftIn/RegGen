#include "RegGen/Lexer/LexerAutomaton.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <numeric>
#include <unordered_map>

#include "RegGen/Common/Text.h"
#include "RegGen/Container/FlatSet.h"
#include "RegGen/Container/SmallVector.h"
#include "RegGen/Lexer/Regex.h"

namespace RG::Lexer {

using PositionLabel = const LabelExpr*;
using PositionSet = FlatSet<PositionLabel>;

using RootExprVec = SmallVector<const RootExpr*>;
using AcceptCategoryLookup =
    std::unordered_map<PositionLabel, const TokenInfo*>;

struct JointRegexTree {
  RootExprVec roots = {};
  AcceptCategoryLookup acc_lookup = {};
};

struct RegexNodeInfo {
  // if the node can match empty string
  bool nullable = false;

  // set of initial positions of the node
  PositionSet first_pos = {};

  // set of terminal positions of the node
  PositionSet last_pos = {};
};

struct RegexEvalResult {
  // extra information for each node
  std::unordered_map<const RegexExpr*, const RegexNodeInfo> info_map = {};

  // follow_pos set for each position
  std::unordered_map<PositionLabel, PositionSet> follow_pos = {};
};

struct RegexExprVisitImpl : public RegexExprVisitor {
  std::unordered_map<const RegexExpr*, const RegexNodeInfo> info_map{};
  std::unordered_map<PositionLabel, PositionSet> follow_pos{};

  const RegexNodeInfo* last_visited_info;

  auto VisitRegexExprVec(const RegexExprVec& exprs)
      -> SmallVector<const RegexNodeInfo*> {
    SmallVector<const RegexNodeInfo*> result;
    for (const auto& expr : exprs) {
      expr->Accept(*this);
      result.push_back(last_visited_info);
    }
    return result;
  }

  auto UpdateNodeInfo(const RegexExpr& expr, const RegexNodeInfo& info)
      -> void {
    info_map.insert({&expr, info});
    last_visited_info = &info_map.at(&expr);
  }

  auto Visit(const RootExpr& expr) -> void override {
    expr.Child()->Accept(*this);
    const auto& child_info = *last_visited_info;
    for (const auto* pos : child_info.last_pos) {
      follow_pos[pos].insert(&expr);
    }
    UpdateNodeInfo(expr, {false, child_info.first_pos, {&expr}});
  }

  auto Visit(const EntityExpr& expr) -> void override {
    UpdateNodeInfo(expr, {false, {&expr}, {&expr}});
  }

  auto Visit(const SequenceExpr& expr) -> void override {
    auto child_info = VisitRegexExprVec(expr.Child());

    bool nullable =
        std::all_of(child_info.begin(), child_info.end(),
                    [](const auto* info) { return info->nullable; });

    PositionSet first_pos = {};
    PositionSet last_pos = {};

    for (const auto* child : child_info) {
      first_pos.insert(child->first_pos.begin(), child->first_pos.end());

      if (!child->nullable) {
        break;
      }
    }

    for (auto it = child_info.rbegin(); it != child_info.rend(); ++it) {
      last_pos.insert((*it)->last_pos.begin(), (*it)->last_pos.end());

      if (!(*it)->nullable) {
        break;
      }
    }

    const RegexNodeInfo* last_child_info = nullptr;
    for (const auto* child : child_info) {
      if (last_child_info) {
        for (const auto* pos : last_child_info->last_pos) {
          follow_pos[pos].insert(child->first_pos.begin(),
                                 child->first_pos.end());
        }
      }
      last_child_info = child;
    }
    UpdateNodeInfo(expr, {nullable, first_pos, last_pos});
  }

  auto Visit(const ChoiceExpr& expr) -> void override {
    auto child_info = VisitRegexExprVec(expr.Child());

    bool nullable =
        std::any_of(child_info.begin(), child_info.end(),
                    [](const auto* info) { return info->nullable; });

    PositionSet first_pos = {};
    PositionSet last_pos = {};

    for (const auto* child : child_info) {
      first_pos.insert(child->first_pos.begin(), child->first_pos.end());
      last_pos.insert(child->last_pos.begin(), child->last_pos.end());
    }

    UpdateNodeInfo(expr, {nullable, first_pos, last_pos});
  }

  auto Visit(const ClosureExpr& expr) -> void override {
    expr.Child()->Accept(*this);
    const auto& child_info = *last_visited_info;

    auto stragegy = expr.Mode();
    if (stragegy != RepetitionMode::Optional) {
      for (const auto* pos : child_info.last_pos) {
        follow_pos[pos].insert(child_info.first_pos.begin(),
                               child_info.first_pos.end());
      }
    }

    auto nullable = stragegy != RepetitionMode::Plus;
    UpdateNodeInfo(expr, {nullable, child_info.first_pos, child_info.last_pos});
  }
};

}  // namespace RG::Lexer