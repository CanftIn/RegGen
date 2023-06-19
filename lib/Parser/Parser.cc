#include "RegGen/Parser/Parser.h"

#include <string>
#include <variant>

#include "RegGen/CodeGen/CppEmitter.h"
#include "RegGen/Common/Format.h"
#include "RegGen/Container/SmallVector.h"
#include "RegGen/Lexer/LexerAutomaton.h"
#include "RegGen/Parser/MetaInfo.h"
#include "RegGen/Parser/ParserAutomaton.h"

namespace RG {

auto BootstrapParser(const std::string& config) -> std::string {
  auto info = ResolveParserInfo(config, nullptr);

  CppEmitter e;

  e.WriteLine("#pragma once");
  e.Include("RegGen/RegGenInclude.h", false);

  e.EmptyLine();
  e.Namespace("RG", [&]() {
    e.EmptyLine();
    e.Comment("Referred Names");
    e.Comment("");

    e.WriteLine("using RG::AST::BasicASTToken;");
    e.WriteLine("using RG::AST::BasicASTEnum;");
    e.WriteLine("using RG::AST::BasicASTObject;");

    e.WriteLine("using RG::AST::ASTVector;");
    e.WriteLine("using RG::AST::ASTOptional;");

    e.WriteLine("using RG::AST::DataBundle;");
    e.WriteLine("using RG::AST::BasicASTTypeProxy;");
    e.WriteLine("using RG::AST::ASTTypeProxyManager;");

    e.WriteLine("using eds::loli::BasicParser;");

    e.EmptyLine();
    e.Comment("Forward declarations");
    e.Comment("");

    e.EmptyLine();
    for (const auto& base_def : info->Bases()) {
      e.WriteLine("class {};", base_def.Name());
    }

    e.EmptyLine();
    for (const auto& class_def : info->Classes()) {
      e.WriteLine("class {};", class_def.Name());
    }

    e.EmptyLine();
    e.Comment("Enum definitions");

    e.EmptyLine();
    for (const auto& enum_def : info->Enums()) {
      e.Enum(enum_def.Name(), "", [&]() {
        for (const auto& item : enum_def.Values()) {
          e.WriteLine("{},", item);
        }
      });
    }

    e.EmptyLine();
    e.Comment("Base definitions");

    e.EmptyLine();
    for (const auto& base_def : info->Bases()) {
      e.Class(base_def.Name(), "public BasicASTObject", [&]() {
        e.WriteLine("public:");

        e.Struct("Visitor", "", [&]() {
          for (const auto& class_def : info->Classes()) {
            if (class_def.BaseType() == &base_def) {
              e.WriteLine("virtual void Visit({}&) = 0;", class_def.Name());
            }
          }
        });

        e.EmptyLine();
        e.WriteLine("virtual void Accept(Visitor&) = 0;");
      });
    }

    e.EmptyLine();
    e.Comment("Class definitions");

    e.EmptyLine();
    for (const auto& class_def : info->Classes()) {
      std::string type_tuple;
      for (const auto& member : class_def.Members()) {
        auto type = member.type.type->Name();

        if (type == "token") {
          type = "BasicASTToken";
        } else if (member.type.type->IsEnum()) {
          type = Format("BasicASTEnum<{}>", type);
        } else if (member.type.type->IsStoredByRef()) {
          type.append("*");
        }

        if (member.type.IsVector()) {
          type = Format("ASTVector<{}>*", type);
        } else if (member.type.IsOptional()) {
          type = Format("ASTOptional<{}>", type);
        }

        // append to tuple
        if (!type_tuple.empty()) {
          type_tuple.append(", ");
        }

        type_tuple.append(type);
      }

      auto* base = class_def.BaseType();
      auto inh = Format("public {}, public DataBundle<{}>",
                        (base ? base->Name() : "BasicASTObject"), type_tuple);
      e.Class(class_def.Name(), inh, [&]() {
        e.WriteLine("public:");

        int index = 0;
        for (const auto& member : class_def.Members()) {
          e.WriteLine("const auto& {}() const {{ return GetItem<{}>(); }}",
                      member.name, index++);
        }

        if (base) {
          e.EmptyLine();
          e.WriteLine(
              "void Accept({}::Visitor& v) override {{ v.Visit(*this); }}",
              base->Name());
        }
      });
    }

    e.EmptyLine();
    e.Comment("Environment");

    e.EmptyLine();
    auto root_name = info->RootVariable().Type().type->Name();
    auto func_name =
        Format("inline BasicParser<{}>::Ptr CreateParser()", root_name);
    e.Block(func_name, [&]() {
      e.WriteLine(
          "static const auto config = \nu8R\"##########(\n{}\n)##########\";",
          config);

      e.Block("static const auto proxy_manager = []()", [&]() {
        e.WriteLine("ASTTypeProxyManager env;");

        e.EmptyLine();
        e.Comment("register enums");
        for (const auto& enum_def : info->Enums()) {
          const auto& name = enum_def.Name();
          e.WriteLine("env.RegisterEnum<{}>(\"{}\");", name, name);
        }

        e.EmptyLine();
        e.Comment("register bases");
        for (const auto& base_def : info->Bases()) {
          const auto& name = base_def.Name();
          e.WriteLine("env.RegisterClass<{}>(\"{}\");", name, name);
        }

        e.EmptyLine();
        e.Comment("register classes");
        for (const auto& class_def : info->Classes()) {
          const auto& name = class_def.Name();
          e.WriteLine("env.RegisterClass<{}>(\"{}\");", name, name);
        }

        e.EmptyLine();
        e.WriteLine("return env;");
      });
      e.WriteLine("();");

      // parser
      e.EmptyLine();
      e.WriteLine("return BasicParser<{}>::Create(config, &proxy_manager);",
                  root_name);
    });
  });

  e.EmptyLine();

  return e.ToString();
}

class ParserContext {
 public:
  ParserContext(Arena& arena) : arena_(arena) {}

  auto StackDepth() const -> int { return state_stack_.size(); }
  auto CurrentState() const -> int {
    return state_stack_.empty() ? 0 : state_stack_.back();
  }

  auto ExecuteShift(int target_state, const AST::ASTItem& value) {
    state_stack_.push_back(target_state);
    ast_stack_.push_back(value);
  }

  auto ExecuteReduce(const ProductionInfo& production) -> AST::ASTItem {
    const auto count = production.Right().size();
    for (auto i = 0; i < count; ++i) {
      state_stack_.pop_back();
    }

    auto ref = ArrayRef<AST::ASTItem>(ast_stack_.data(), ast_stack_.size())
                   .take_back(count);
    auto result = production.Handle()->Invoke(arena_, ref);

    for (auto i = 0; i < count; ++i) {
      ast_stack_.pop_back();
    }

    return result;
  }

  auto Finalize() -> AST::ASTItem {
    assert(StackDepth() == 1);
    auto result = ast_stack_.back();
    state_stack_.clear();
    ast_stack_.clear();

    return result;
  }

 private:
  Arena& arena_;

  SmallVector<int> state_stack_ = {};
  SmallVector<AST::ASTItem> ast_stack_ = {};
};

GenericParser::GenericParser(const std::string& config,
                             const AST::ASTTypeProxyManager* env) {
  Initialize(config, env);
}

auto TranslateAction(PdaEdge action) -> ParserAction {
  struct Visitor {
    auto operator()(PdaEdgeReduce edge) -> ParserAction {
      return ActionReduce{edge.production};
    }
    auto operator()(PdaEdgeShift edge) -> ParserAction {
      return ActionShift{edge.target->Id()};
    }
  };

  return visit(Visitor{}, action);
}

auto GenericParser::Initialize(const std::string& config,
                               const AST::ASTTypeProxyManager* env) -> void {
  assert(!config.empty() && env != nullptr);

  info_ = ResolveParserInfo(config, env);

  auto dfa = BuildLexerAutomaton(*info_);
  auto pda = BuildLALRAutomaton(*info_);

  token_num_ = info_->Tokens().size() + info_->IgnoredTokens().size();
  term_num_ = info_->Tokens().size();
  nonterm_num_ = info_->Variables().size();

  dfa_state_num_ = dfa->StateCount();
  pda_state_num_ = pda->States().size();

  // lexing table
  acc_token_lookup_.initialize(dfa->StateCount(), nullptr);
  lexing_table_.initialize(128 * dfa_state_num_, -1);

  // parsing table
  eof_action_table_.initialize(pda_state_num_, ActionError{});
  action_table_.initialize(pda_state_num_ * term_num_, ActionError{});
  goto_table_.initialize(pda_state_num_ * nonterm_num_, -1);

  for (int id = 0; id < dfa_state_num_; ++id) {
    const auto* const state = dfa->LookupState(id);

    acc_token_lookup_[id] = state->acc_token;
    for (const auto edge : state->transitions) {
      lexing_table_[id * 128 + edge.first] = edge.second->id;
    }
  }

  for (int src_state_id = 0; src_state_id < pda_state_num_; ++src_state_id) {
    const auto* state = pda->LookupState(src_state_id);

    if (state->EofAction()) {
      eof_action_table_[src_state_id] = TranslateAction(*state->EofAction());
    }

    for (const auto& pair : state->ActionMap()) {
      const auto tok_id = pair.first->Id();

      action_table_[src_state_id * term_num_ + tok_id] =
          TranslateAction(pair.second);
    }

    for (const auto& pair : state->GotoMap()) {
      const auto var_id = pair.first->Id();

      goto_table_[src_state_id * nonterm_num_ + var_id] = pair.second->Id();
    }
  }
}

auto GenericParser::Parse(Arena& arena, const std::string& data)
    -> AST::ASTItem {
  ParserContext ctx{arena};
  int offset = 0;

  // tokenize and feed parser while not exhausted
  while (offset < data.length()) {
    auto tok = LoadToken(data, offset);

    // update offset
    offset = tok.Offset() + tok.Length();

    // throw for invalid token
    if (!tok.IsValid()) {
      throw ParserInternalError{"GenericParser: invalid token encountered"};
    }
    // ignore tokens in blacklist
    if (tok.Tag() >= term_num_) {
      continue;
    }

    FeedParserContext(ctx, tok);
  }

  FeedParserContext(ctx, {});

  return ctx.Finalize();
}

auto GenericParser::LoadToken(std::string_view data, int offset)
    -> AST::BasicASTToken {
  auto last_acc_len = 0;
  const TokenInfo* last_acc_token = nullptr;

  auto state = LexerInitialState();
  for (int i = offset; i < data.length(); ++i) {
    state = LookupLexingTransition(state, data[i]);

    if (!VerifyLexingState(state)) {
      break;
    } else if (const auto* acc_token = LookupAcceptedToken(state); acc_token) {
      last_acc_len = i - offset + 1;
      last_acc_token = acc_token;
    }
  }

  if (last_acc_len != 0) {
    return AST::BasicASTToken{offset, last_acc_len, last_acc_token->Id()};
  } else {
    return AST::BasicASTToken{};
  }
}

auto GenericParser::ForwardParserAction(ParserContext& ctx, ActionShift action,
                                        const AST::BasicASTToken& tok)
    -> ActionExecutionResult {
  assert(tok.IsValid());

  ctx.ExecuteShift(action.target_state, tok);
  return ActionExecutionResult::Consumed;
}

auto GenericParser::ForwardParserAction(ParserContext& ctx, ActionReduce action,
                                        const AST::BasicASTToken& tok)
    -> ActionExecutionResult {
  auto folded = ctx.ExecuteReduce(*action.production);

  auto nonterm_id = action.production->Left()->Id();
  auto src_state = ctx.CurrentState();
  auto target_state = LookupParsingGoto(src_state, nonterm_id);

  ctx.ExecuteShift(target_state, folded);

  if (!tok.IsValid() && ctx.StackDepth() == 1 &&
      action.production->Left() == &info_->RootVariable()) {
    return ActionExecutionResult::Consumed;
  } else {
    return ActionExecutionResult::Hungry;
  }
}
auto GenericParser::ForwardParserAction(ParserContext& /*ctx*/,
                                        ActionError /*action*/,
                                        const AST::BasicASTToken& /*tok*/)
    -> ActionExecutionResult {
  return ActionExecutionResult::Error;
}

auto GenericParser::FeedParserContext(ParserContext& ctx,
                                      const AST::BasicASTToken& tok) -> void {
  while (true) {
    auto cur_state = ctx.CurrentState();
    auto action = tok.IsValid() ? LookupParserAction(cur_state, tok.Tag())
                                : LookupParserActionOnEof(cur_state);

    auto action_result =
        visit([&](auto x) { return ForwardParserAction(ctx, x, tok); }, action);

    if (action_result == ActionExecutionResult::Error) {
      throw ParserInternalError{"parsing error"};
    } else if (action_result == ActionExecutionResult::Consumed) {
      break;
    }
  }
}

}  // namespace RG