#ifndef REGGEN_PARSER_GRAMMAR_H
#define REGGEN_PARSER_GRAMMAR_H

#include <map>
#include <memory>
#include <utility>

#include "RegGen/Container/FlatSet.h"
#include "RegGen/Container/SmallVector.h"
#include "RegGen/Parser/TypeInfo.h"

namespace RG {

class ParserState;

class Symbol;
class Terminal;
class Nonterminal;
class Production;

using TerminalSet = FlatSet<Terminal*>;
using SymbolVec = SmallVector<Symbol*>;

using SymbolKey = std::tuple<const SymbolInfo*, const ParserState*>;

class Symbol {
 public:
  Symbol(const SymbolInfo* key, const ParserState* version)
      : key_(key), version_(version) {}

  virtual ~Symbol() = default;

  auto Key() const -> const auto& { return key_; }
  auto Version() const -> const auto& { return version_; }

  auto AsTerminal() -> Terminal*;
  auto AsNonterminal() -> Nonterminal*;

 private:
  const SymbolInfo* key_;
  const ParserState* version_;
};

class Terminal : public Symbol {
 public:
  Terminal(const TokenInfo* info, const ParserState* version)
      : info_(info), Symbol(info, version) {}

  auto Info() const -> const auto& { return info_; }

 private:
  const TokenInfo* info_;
};

class Nonterminal : public Symbol {
 public:
  Nonterminal(const VariableInfo* info, const ParserState* version)
      : info_(info), Symbol(info, version) {}

  auto Info() const -> const auto& { return info_; }
  auto Productions() const -> const auto& { return productions_; }

  auto MayProduceEpsilon() const -> const auto& { return may_produce_epsilon_; }
  auto MayPreceedEof() const -> const auto& { return may_preceed_eof_; }
  auto FirstSet() const -> const auto& { return first_set_; }
  auto FollowSet() const -> const auto& { return follow_set_; }

 private:
  friend class GrammarBuilder;

  const VariableInfo* info_;
  SmallVector<Production*> productions_{};

  bool may_produce_epsilon_{false};
  bool may_preceed_eof_{false};

  TerminalSet first_set_ = {};
  TerminalSet follow_set_ = {};
};

class Production {
 public:
  Production(const ProductionInfo* info, Nonterminal* lhs, SymbolVec rhs)
      : info_(info), lhs_(lhs), rhs_(std::move(rhs)) {}

  auto Info() const -> const auto& { return info_; }
  auto Left() const -> const auto& { return lhs_; }
  auto Right() const -> const auto& { return rhs_; }

 private:
  friend class GrammarBuilder;

  const ProductionInfo* info_;

  Nonterminal* lhs_;
  SymbolVec rhs_;
};

class Grammar {
 public:
  auto RootSymbol() const -> const auto& { return root_symbol_; }
  auto Terminals() const -> const auto& { return terms_; }
  auto Nonterminals() const -> const auto& { return nonterms_; }
  auto Productions() const -> const auto& { return productions_; }

  auto LookupTerminal(SymbolKey key) -> Terminal*;
  auto LookupNonterminal(SymbolKey key) -> Nonterminal*;

 private:
  friend class GrammarBuilder;

  Nonterminal* root_symbol_;

  std::map<SymbolKey, Terminal> terms_;
  std::map<SymbolKey, Nonterminal> nonterms_;

  std::vector<std::unique_ptr<Production>> productions_;
};

class GrammarBuilder {
 public:
  auto MakeTerminal(const TokenInfo* info, const ParserState* version)
      -> Terminal*;
  auto MakeNonterminal(const VariableInfo* info, const ParserState* version)
      -> Nonterminal*;
  auto MakeGenericSymbol(const SymbolInfo* info, const ParserState* version)
      -> Symbol*;

  auto CreateProduction(const ProductionInfo* info, Nonterminal* lhs,
                        const SymbolVec& rhs) -> void;

  auto Build(Nonterminal* root) -> std::unique_ptr<Grammar>;

 private:
  auto ComputeFirstSet() -> void;
  auto ComputeFollowSet() -> void;

  std::unique_ptr<Grammar> site_ = std::make_unique<Grammar>();
};

}  // namespace RG

#endif  // REGGEN_PARSER_GRAMMAR_H