#ifndef REGGEN_LEXER_LEXER_AUTOMATON_H
#define REGGEN_LEXER_LEXER_AUTOMATON_H

#include <functional>
#include <optional>

#include "RegGen/Lexer/Regex.h"
#include "RegGen/Parser/TypeInfo.h"

namespace RG::Lexer {

struct DfaState {
  int id;
  const TokenInfo* acc_token;
  std::unordered_map<int, DfaState*> transitions;

 public:
  DfaState(int id, const TokenInfo* acc_token = nullptr)
      : id(id), acc_token(acc_token) {}
};

class LexerAutomaton : NonCopyable, NonMovable {
 public:
  auto StateCount() const -> int { return states_.size(); }

  auto LookupState(int id) const -> const DfaState* {
    return states_.at(id).get();
  }

  auto NewState(const TokenInfo* acc_category = nullptr) -> DfaState* {
    int id = StateCount();

    return states_.emplace_back(std::make_unique<DfaState>(id, acc_category))
        .get();
  }

  auto NewTransition(DfaState* src, DfaState* target, int ch) -> void {
    assert(ch >= 0 && ch < 128);
    assert(src->transitions.count(ch) == 0);
    src->transitions.insert_or_assign(ch, target);
  }

 private:
  SmallVector<std::unique_ptr<DfaState>> states_;
};

auto BuildLexerAutomaton(const MetaInfo& info)
    -> std::unique_ptr<const LexerAutomaton>;

}  // namespace RG::Lexer

#endif  // REGGEN_LEXER_LEXER_AUTOMATON_H