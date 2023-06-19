#ifndef REGGEN_PARSER_PARSER_AUTOMATON_H
#define REGGEN_PARSER_PARSER_AUTOMATON_H

#include <cassert>
#include <functional>
#include <map>
#include <optional>
#include <variant>

#include "RegGen/Container/FlatSet.h"
#include "RegGen/Container/SmallVector.h"
#include "RegGen/Parser/MetaInfo.h"
#include "RegGen/Parser/TypeInfo.h"

namespace RG {

class ParserState;

class ParserItem {
 public:
  struct Less {
    auto operator()(const ParserItem& lhs, const ParserItem& rhs) const
        -> bool {
      if (lhs.production_ < rhs.production_) {
        return true;
      } else if (lhs.production_ > rhs.production_) {
        return false;
      }
      return lhs.cursor_ < rhs.cursor_;
    }
  };

  ParserItem(const ProductionInfo* production, int cursor)
      : production_(production), cursor_(cursor) {
    assert(production != nullptr);
    assert(cursor >= 0 && cursor <= production->Right().size());
  }

  auto Production() const -> const auto& { return production_; }
  auto Cursor() const -> const auto& { return cursor_; }

  auto CreateSuccessor() const -> ParserItem {
    assert(!IsFinalized());
    return ParserItem{production_, cursor_ + 1};
  }

  auto NextSymbol() const -> const SymbolInfo* {
    if (IsFinalized()) {
      return nullptr;
    }

    return production_->Right()[cursor_];
  }

  auto IsKernel() const -> bool { return cursor_ > 0; }

  auto IsFinalized() const -> bool {
    return cursor_ == production_->Right().size();
  }

 private:
  const ProductionInfo* production_;
  int cursor_;
};

using ItemSet = FlatSet<ParserItem, ParserItem::Less>;

struct PdaEdgeShift {
  const ParserState* target;
};
struct PdaEdgeReduce {
  const ProductionInfo* production;
};

using PdaEdge = std::variant<PdaEdgeShift, PdaEdgeReduce>;

class ParserState {
 public:
  ParserState(int id) : id_(id) {}

  auto Id() const -> const auto& { return id_; }
  auto EofAction() const -> const auto& { return eof_action_; }
  auto ActionMap() const -> const auto& { return action_map_; }
  auto GotoMap() const -> const auto& { return goto_map_; }

  auto RegisterShift(const ParserState* dest, const SymbolInfo* s) -> void;

  auto RegisterReduce(const ProductionInfo* p, const TokenInfo* tok) -> void;
  auto RegisterReduceOnEof(const ProductionInfo* p) -> void;

 private:
  int id_;

  std::optional<PdaEdgeReduce> eof_action_;
  std::unordered_map<const TokenInfo*, PdaEdge> action_map_;
  std::unordered_map<const VariableInfo*, const ParserState*> goto_map_;
};

class ParserAutomaton {
 public:
  auto StateCount() const -> int { return states_.size(); }
  auto LookupState(int id) const -> const ParserState* { return ptrs_.at(id); }

  auto States() const { return states_; }

  auto MakeState(const ItemSet& items) -> ParserState*;
  auto EnumerateState(
      std::function<void(const ItemSet&, ParserState&)> callback) -> void;

 private:
  SmallVector<ParserState*> ptrs_;
  std::map<ItemSet, ParserState> states_;
};

auto BuildLALRAutomaton(
    const MetaInfo& info) -> std::unique_ptr<const ParserAutomaton>;

}  // namespace RG

#endif  // REGGEN_PARSER_PARSER_AUTOMATON_H