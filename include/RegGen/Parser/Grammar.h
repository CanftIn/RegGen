#ifndef REGGEN_PARSER_GRAMMAR_H
#define REGGEN_PARSER_GRAMMAR_H

#include <map>
#include <memory>

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

 private:
  friend class GrammarBuilder;

  const VariableInfo* info_;
  SmallVector<Production*> productions_{};

  bool may_produce_epsilon_{false};
};

}  // namespace RG

#endif  // REGGEN_PARSER_GRAMMAR_H