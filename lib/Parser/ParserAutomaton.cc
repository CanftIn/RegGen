#include "RegGen/Parser/ParserAutomaton.h"

#include <cassert>
#include <map>
#include <set>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

#include "RegGen/Container/FlatSet.h"
#include "RegGen/Container/SmallVector.h"
#include "RegGen/Parser/Grammar.h"

namespace RG {

auto ParserState::RegisterShift(const ParserState* dest, const SymbolInfo* s)
    -> void {
  if (const auto* tok = s->AsToken(); tok) {
    assert(action_map_.count(tok) == 0);
    action_map_.insert_or_assign(tok, PdaEdgeShift{dest});
  } else {
    const auto* var = s->AsVariable();

    assert(goto_map_.count(var) == 0);
    goto_map_.insert_or_assign(var, dest);
  }
}

auto ParserState::RegisterReduce(const ProductionInfo* p, const TokenInfo* tok)
    -> void {
  assert(action_map_.count(tok) == 0);
  action_map_.insert_or_assign(tok, PdaEdgeReduce{p});
}

auto ParserState::RegisterReduceOnEof(const ProductionInfo* p) -> void {
  assert(!eof_action_.has_value());
  eof_action_ = PdaEdgeReduce{p};
}

auto ParserAutomaton::MakeState(const ItemSet& items) -> ParserState* {
  auto iter = states_.find(items);
  if (iter == states_.end()) {
    auto id = static_cast<int>(states_.size());
    iter = states_.try_emplace(iter, items, ParserState{id});

    ptrs_.push_back(&iter->second);
  }

  return &(iter->second);
}

auto ParserAutomaton::EnumerateState(
    std::function<void(const ItemSet&, ParserState&)> callback) -> void {
  for (auto& pair : states_) {
    callback(pair.first, pair.second);
  }
}

template <typename F>
auto EnumerateClosureItems(const MetaInfo& /*info*/, const ItemSet& kernel,
                           F callback) -> void {
  static_assert(std::is_invocable_v<F, ParserItem>);

  auto added = std::unordered_set<const VariableInfo*>{};
  auto to_be_visited = SmallVector<const VariableInfo*>{};

  const auto try_register_candidate = [&](const SymbolInfo* s) {
    if (const auto* candidate = s->AsVariable(); candidate) {
      if (added.count(candidate) == 0) {
        added.insert(candidate);
        to_be_visited.push_back(candidate);
      }
    }
  };

  // visit kernel items and record nonterms for closure calculation
  for (const auto& item : kernel) {
    callback(item);

    // Item{ A -> \alpha . }: skip it
    // Item{ A -> \alpha . B \gamma }: queue unvisited nonterm B for further
    // expansion
    if (const auto* s = item.NextSymbol(); s) {
      try_register_candidate(s);
    }
  }

  // genreate and visit non-kernel items recursively
  while (!to_be_visited.empty()) {
    const auto* variable = to_be_visited.back();
    to_be_visited.pop_back();

    for (const auto& p : variable->Productions()) {
      callback(ParserItem{p, 0});

      if (!p->Right().empty()) {
        try_register_candidate(p->Right().front());
      }
    }
  }
}

auto ComputeGotoItems(const MetaInfo& info, const ItemSet& src,
                      const SymbolInfo* s) -> ItemSet {
  ItemSet new_state;

  EnumerateClosureItems(info, src, [&](ParserItem item) {
    // for Item A -> \alpha . B \beta where B == s, advance the cursor
    if (item.NextSymbol() == s) {
      new_state.insert(item.CreateSuccessor());
    }
  });

  return new_state;
}

auto GenerateInitialItems(const MetaInfo& info) -> ItemSet {
  ItemSet result;
  for (auto* p : info.RootSymbol().Productions()) {
    result.insert(ParserItem{p, 0});
  }

  return result;
}

template <typename F>
void EnumerateSymbols(const MetaInfo& info, F callback) {
  static_assert(std::is_invocable_v<F, const SymbolInfo*>);

  for (const auto& tok : info.Tokens()) {
    callback(&tok);
  }

  for (const auto& var : info.Variables()) {
    callback(&var);
  }
}

auto BootstrapParsingAutomaton(const MetaInfo& info) {
  auto pda = std::make_unique<ParserAutomaton>();

  auto initial_set = GenerateInitialItems(info);

  for (std::deque<ItemSet> unprocessed{initial_set}; !unprocessed.empty();
       unprocessed.pop_front()) {
    const auto& src_items = unprocessed.front();
    auto* const src_state = pda->MakeState(src_items);

    EnumerateSymbols(info, [&](const SymbolInfo* s) {
      // calculate the target state for symbol s
      auto dest_items = ComputeGotoItems(info, src_items, s);

      // empty set of item is not a valid state
      if (dest_items.empty()) {
        return;
      }

      // compute target state
      auto old_state_cnt = pda->States().size();
      ParserState* dest_state = pda->MakeState(dest_items);
      if (pda->States().size() > old_state_cnt) {
        unprocessed.push_back(dest_items);
      }

      src_state->RegisterShift(dest_state, s);
    });
  }

  return pda;
}

auto LookupTargetState(const ParserState* src, const SymbolInfo* s)
    -> const ParserState* {
  if (const auto* tok = s->AsToken(); tok) {
    return std::get<PdaEdgeShift>(src->ActionMap().at(tok)).target;
  } else {
    return src->GotoMap().at(s->AsVariable());
  }
}

auto CreateExtendedGrammar(const MetaInfo& info, ParserAutomaton& pda)
    -> std::unique_ptr<Grammar> {
  GrammarBuilder builder;

  // extend symbols
  pda.EnumerateState([&](const ItemSet& /*items*/, ParserState& state) {
    // extend nonterms
    for (const auto& goto_edge : state.GotoMap()) {
      const auto* var_info = goto_edge.first;
      const auto* version = goto_edge.second;

      builder.MakeNonterminal(var_info, version);
    }

    // extend terms
    for (const auto& action_edge : state.ActionMap()) {
      const auto* tok_info = action_edge.first;
      const auto* version = std::get<PdaEdgeShift>(action_edge.second).target;

      builder.MakeTerminal(tok_info, version);
    }
  });

  auto* new_root = builder.MakeNonterminal(&info.RootVariable(), nullptr);

  pda.EnumerateState([&](const ItemSet& items, ParserState& state) {
    EnumerateClosureItems(info, items, [&](ParserItem item) {
      if (item.IsKernel()) {
        return;
      }

      // shortcuts
      const auto& production_info = item.Production();

      // map left-hand side
      Nonterminal* lhs = new_root;
      if (state.Id() != 0 || production_info->Left() != new_root->Info()) {
        // TODO: remove hard-coded state id 0

        // exclude root nonterm because its version is set to nullptr
        const auto* version =
            LookupTargetState(&state, production_info->Left());
        lhs = builder.MakeNonterminal(production_info->Left(), version);
      }

      // map right-hand side
      SmallVector<Symbol*> rhs = {};

      const ParserState* current_state = &state;
      for (auto* rhs_elem : production_info->Right()) {
        const auto* next_state = LookupTargetState(current_state, rhs_elem);
        assert(next_state != nullptr);

        if (const auto* tok = rhs_elem->AsToken(); tok) {
          rhs.push_back(builder.MakeTerminal(tok, next_state));
        } else {
          const auto* var = rhs_elem->AsVariable();

          rhs.push_back(builder.MakeNonterminal(var, next_state));
        }

        current_state = next_state;
      }

      // create production
      builder.CreateProduction(production_info, lhs, rhs);
    });
  });

  return builder.Build(new_root);
}

auto BuildLALRAutomaton(const MetaInfo& info)
    -> std::unique_ptr<const ParserAutomaton> {
  auto pda = BootstrapParsingAutomaton(info);
  auto ext_grammar = CreateExtendedGrammar(info, *pda);

  // (state where reduction is done, production)
  using LocatedProduction =
      std::tuple<const ParserState*, const ProductionInfo*>;

  // merge follow set
  std::set<LocatedProduction> merged_ending;
  std::map<LocatedProduction, FlatSet<const TokenInfo*>> merged_follow;
  for (const auto& p : ext_grammar->Productions()) {
    const auto& lhs = p->Left();
    const auto& rhs = p->Right();

    auto key = LocatedProduction{rhs.back()->Version(), p->Info()};

    // normalized ending
    if (lhs->MayPreceedEof()) {
      merged_ending.insert(key);
    }

    // normalized FOLLOW set
    auto& follow = merged_follow[key];
    for (auto* term : lhs->FollowSet()) {
      follow.insert(term->Info());
    }
  }

  // register reductions
  pda->EnumerateState([&](const ItemSet& items, ParserState& state) {
    for (auto item : items) {
      const auto& production = item.Production();
      auto key = LocatedProduction{&state, production};

      if (item.IsFinalized()) {
        // EOF
        if (merged_ending.count(key) > 0) {
          state.RegisterReduceOnEof(production);
        }

        // for all term in FOLLOW do reduce
        for (const auto* term : merged_follow.at(key)) {
          state.RegisterReduce(production, term);
        }
      }
    }
  });

  return pda;
}
}  // namespace RG