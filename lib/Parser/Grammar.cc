#include "RegGen/Parser/Grammar.h"

#include <cassert>

namespace RG {

auto Symbol::AsTerminal() -> Terminal* {
  return key_->IsToken() ? reinterpret_cast<Terminal*>(this) : nullptr;
}

auto Symbol::AsNonterminal() -> Nonterminal* {
  return key_->IsVariable() ? reinterpret_cast<Nonterminal*>(this) : nullptr;
}

auto Grammar::LookupTerminal(SymbolKey key) -> Terminal* {
  auto it = terms_.find(key);

  return it != terms_.end() ? &it->second : nullptr;
}
auto Grammar::LookupNonterminal(SymbolKey key) -> Nonterminal* {
  auto iter = nonterms_.find(key);

  return iter != nonterms_.end() ? &iter->second : nullptr;
}

auto GrammarBuilder::MakeTerminal(const TokenInfo* info,
                                  const ParserState* version) -> Terminal* {
  auto& lookup = site_->terms_;

  auto key = SymbolKey{info, version};
  auto iter = lookup.find(key);
  if (iter == lookup.end()) {
    iter = lookup.try_emplace(iter, key, Terminal{info, version});
  }

  return &iter->second;
}

auto GrammarBuilder::MakeNonterminal(const VariableInfo* info,
                                     const ParserState* version)
    -> Nonterminal* {
  auto& lookup = site_->nonterms_;

  auto key = SymbolKey{info, version};
  auto iter = lookup.find({info, version});
  if (iter == lookup.end()) {
    iter = lookup.try_emplace(iter, key, Nonterminal{info, version});
  }

  return &iter->second;
}

auto GrammarBuilder::MakeGenericSymbol(const SymbolInfo* info,
                                       const ParserState* version) -> Symbol* {
  if (const auto* tok = info->AsToken(); tok) {
    return MakeTerminal(tok, version);
  } else {
    const auto* var = info->AsVariable();
    assert(var != nullptr);

    return MakeNonterminal(var, version);
  }
}

auto GrammarBuilder::CreateProduction(const ProductionInfo* info,
                                      Nonterminal* lhs, const SymbolVec& rhs)
    -> void {
  auto* result = site_->productions_
                     .emplace_back(std::make_unique<Production>(info, lhs, rhs))
                     .get();

  lhs->productions_.push_back(result);
}

auto GrammarBuilder::Build(Nonterminal* root) -> std::unique_ptr<Grammar> {
  site_->root_symbol_ = root;

  ComputeFirstSet();
  ComputeFollowSet();

  return std::move(site_);
}

// try to insert FIRST SET of s into output
static auto TryInsertFIRST(TerminalSet& output, Symbol* s) -> bool {
  // remember output's size
  const auto old_output_sz = output.size();

  // try insert terminals that start s into output
  if (auto* term = s->AsTerminal(); term) {
    output.insert(term);
  } else {
    const auto& source_set = s->AsNonterminal()->FirstSet();
    output.insert(source_set.begin(), source_set.end());
  }

  // return if output is changed
  return output.size() != old_output_sz;
}

// try to insert FOLLOW SET of s into output
static auto TryInsertFOLLOW(TerminalSet& output, const Nonterminal* s) -> bool {
  // remember output's size
  const auto old_output_sz = output.size();

  // try insert terminals that may follow s into output
  output.insert(s->FollowSet().begin(), s->FollowSet().end());

  // return if output is changed
  return output.size() != old_output_sz;
}

auto GrammarBuilder::ComputeFirstSet() -> void {
  // iteratively compute first set until it's not growing
  for (auto growing = true; growing;) {
    growing = false;

    for (const auto& production : site_->productions_) {
      auto& lhs = production->lhs_;

      bool may_produce_epsilon = true;
      for (const auto rhs_elem : production->rhs_) {
        growing |= TryInsertFIRST(lhs->first_set_, rhs_elem);

        // break on first non-epsilon-derivable symbol

        if (rhs_elem->AsTerminal() ||
            !rhs_elem->AsNonterminal()->may_produce_epsilon_) {
          may_produce_epsilon = false;
          break;
        }
      }

      if (may_produce_epsilon && !lhs->may_produce_epsilon_) {
        // as the iteration didn't break early
        // it may produce epsilon symbol
        // NOTE empty production also indicates epsilon
        growing = true;
        lhs->may_produce_epsilon_ = true;
      }
    }
  }
}

auto GrammarBuilder::ComputeFollowSet() -> void {
  // NOTE root symbol always preceeds eof
  site_->root_symbol_->may_preceed_eof_ = true;

  // iteratively compute follow set until it's not growing
  for (auto growing = true; growing;) {
    growing = false;

    for (const auto& production : site_->productions_) {
      // shortcuts alias
      const auto& lhs = production->lhs_;
      const auto& rhs = production->rhs_;

      // epsilon_path is set false when any non-nullable symbol is encountered
      bool epsilon_path = true;
      for (auto iter = rhs.rbegin(); iter != rhs.rend(); ++iter) {
        const auto& current_symbol = *iter;

        // process lookahead symbol
        if (const auto next_iter = next(iter); next_iter != rhs.rend()) {
          const auto next_symbol = *next_iter;
          if (auto* nonterm = next_symbol->AsNonterminal(); nonterm) {
            growing |= TryInsertFIRST(nonterm->follow_set_, current_symbol);
          }
        }

        // process current symbol
        if (epsilon_path) {
          if (auto* current_nonterm = current_symbol->AsNonterminal();
              current_nonterm) {
            if (!current_nonterm->may_produce_epsilon_) {
              epsilon_path = false;
            }

            // propagate may_preceed_eof on epsilon path
            if (!current_nonterm->may_preceed_eof_ && lhs->may_preceed_eof_) {
              growing = true;
              current_nonterm->may_preceed_eof_ = true;
            }

            // propagate follow_set on epsilon path
            growing |= TryInsertFOLLOW(current_nonterm->follow_set_, lhs);
          } else {
            epsilon_path = false;
          }
        }
      }
    }
  }
}
}  // namespace RG