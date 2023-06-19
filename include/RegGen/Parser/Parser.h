#ifndef REGGEN_PARSER_PARSER_H
#define REGGEN_PARSER_PARSER_H

#include <memory>

#include "RegGen/AST/ASTBasic.h"
#include "RegGen/Container/Arena.h"
#include "RegGen/Parser/Action.h"
#include "RegGen/Parser/MetaInfo.h"

namespace RG {

auto BootstrapParser(const std::string& config) -> std::string;

class ParserContext;

class GenericParser {
 public:
  GenericParser(const std::string& config, const AST::ASTTypeProxyManager* env);

  auto GrammarInfo() const -> const auto& { return *info_; }

  auto Initialize(const std::string& config,
                  const AST::ASTTypeProxyManager* env) -> void;

  auto Parse(Arena& arena, const std::string& data) -> AST::ASTItem;

 private:
  auto LexerInitialState() const -> int { return 0; }
  auto ParserInitialState() const -> int { return 0; }

  auto VerifyCharacter(int ch) const -> bool { return ch >= 0 && ch < 128; }
  auto VerifyLexingState(int state) const -> bool {
    return state >= 0 && state < dfa_state_num_;
  }
  auto VerifyParsingState(int state) const -> bool {
    return state >= 0 && state < pda_state_num_;
  }

  auto LookupLexingTransition(int state, int ch) const -> int {
    assert(VerifyLexingState(state) && VerifyCharacter(ch));
    return lexing_table_[128 * state + ch];
  }
  auto LookupAcceptedToken(int state) const -> const TokenInfo* {
    assert(VerifyLexingState(state));
    return acc_token_lookup_[state];
  }
  auto LookupParserAction(int state, int term_id) const -> ParserAction {
    assert(VerifyParsingState(state) && term_id >= 0 && term_id < term_num_);
    return action_table_[term_num_ * state + term_id];
  }
  auto LookupParserActionOnEof(int state) const -> ParserAction {
    assert(VerifyParsingState(state));
    return eof_action_table_[state];
  }
  auto LookupParsingGoto(int state, int nonterm_id) const -> int {
    assert(VerifyParsingState(state) && nonterm_id >= 0 &&
           nonterm_id <= nonterm_num_);
    return goto_table_[nonterm_num_ * state + nonterm_id];
  }

  auto LoadToken(std::string_view data, int offset) -> AST::BasicASTToken;

  auto ForwardParserAction(ParserContext& ctx, ActionShift action,
                           const AST::BasicASTToken& tok)
      -> ActionExecutionResult;
  auto ForwardParserAction(ParserContext& ctx, ActionReduce action,
                           const AST::BasicASTToken& tok)
      -> ActionExecutionResult;
  auto ForwardParserAction(ParserContext& ctx, ActionError action,
                           const AST::BasicASTToken& tok)
      -> ActionExecutionResult;

  void FeedParserContext(ParserContext& ctx, const AST::BasicASTToken& tok);

 private:
  std::unique_ptr<MetaInfo> info_;

  // parser
  int token_num_;
  int term_num_;
  int nonterm_num_;

  int dfa_state_num_;
  int pda_state_num_;

  HeapArray<const TokenInfo*> acc_token_lookup_;  // 1 column, token_num_ rows
  HeapArray<int> lexing_table_;  // 128 columns, dfa_state_num_ rows

  HeapArray<ParserAction>
      action_table_;  // term_num_ columns, pda_state_num_ rows
  HeapArray<ParserAction> eof_action_table_;  // 1 column, pda_state_num_ rows
  HeapArray<int> goto_table_;  // nonterm_num_ columns, pda_state_num_ rows
};

template <typename T>
class BasicParser {
 public:
  using Ptr = std::unique_ptr<BasicParser>;
  using ResultType = typename AST::ASTTypeTrait<T>::StoreType;

  auto Parse(Arena& arena, const std::string& data) -> ResultType {
    auto result = parser_->Parse(arena, data);

    return result.Extract<ResultType>();
  }

  static auto Create(const std::string& config,
                     const AST::ASTTypeProxyManager* env) -> Ptr {
    auto result = std::make_unique<BasicParser<T>>();
    result->parser_ = std::make_unique<GenericParser>(config, env);

    return result;
  }

 private:
  std::unique_ptr<GenericParser> parser_;
};

}  // namespace RG

#endif  // REGGEN_PARSER_PARSER_H