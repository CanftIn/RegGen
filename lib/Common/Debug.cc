#include "RegGen/Common/Debug.h"

#include <sstream>

#include "RegGen/Common/Format.h"

namespace RG {

auto ToStringToken(const MetaInfo& info, int id) -> std::string {
  if (id == -1) {
    return "UNACCEPTED";
  }

  auto t = info.Tokens().size();
  if (id < t) {
    return info.Tokens()[id].Name();
  } else {
    return info.IgnoredTokens()[id - t].Name();
  }
}

auto ToStringVariable(const MetaInfo& info, int id) -> std::string {
  return info.Variables()[id].Name();
}

auto ToStringProduction(const ProductionInfo& p) -> std::string {
  std::stringstream buf;
  buf << p.Left()->Name();
  buf << " :=";

  for (const auto& rhs_elem : p.Right()) {
    buf << " ";
    buf << rhs_elem->Name();
  }

  return buf.str();
}

auto ToStringParserAction(const MetaInfo& /*info*/, PdaEdge action)
    -> std::string {
  if (std::holds_alternative<PdaEdgeShift>(action)) {
    return Format("shift to {}", std::get<PdaEdgeShift>(action).target->Id());
  } else if (std::holds_alternative<PdaEdgeReduce>(action)) {
    return Format(
        "reduce ({})",
        ToStringProduction(*std::get<PdaEdgeReduce>(action).production));
  } else {
    throw 0;
  }
}

auto PrintParsingMetaInfo(const MetaInfo& info) -> void {
  PrintFormatted("[Grammar]\n");

  PrintFormatted("tokens:\n");
  for (const auto& tok : info.Tokens()) {
    PrintFormatted("  {}\n", tok.Name());
  }

  PrintFormatted("\n");
  PrintFormatted("ignores:\n");
  for (const auto& tok : info.IgnoredTokens()) {
    PrintFormatted("  {}\n", tok.Name());
  }

  PrintFormatted("\n");
  PrintFormatted("variables:\n");
  for (const auto& var : info.Variables()) {
    PrintFormatted("  {}\n", var.Name());
  }

  PrintFormatted("\n");
  PrintFormatted("productions:\n");
  for (const auto& p : info.Productions()) {
    PrintFormatted("{}\n", ToStringProduction(p));
  }
}

auto PrintGrammar(const Grammar& g) -> void {
  PrintFormatted("Extended Productions:\n");
  for (const auto& p : g.Productions()) {
    auto ver = p->Left()->Version() ? p->Left()->Version()->Id() : -1;
    PrintFormatted("{}_{} :=", p->Left()->Info()->Name(), ver);
    for (const auto& s : p->Right()) {
      PrintFormatted(" {}_{}", s->Key()->Name(), s->Version()->Id());
    }
    PrintFormatted("\n");
  }
  PrintFormatted("\n");

  PrintFormatted("Predicative Sets\n");
  for (const auto& pair : g.Nonterminals()) {
    const auto& var = pair.second;

    auto ver = var.Version() ? var.Version()->Id() : -1;
    PrintFormatted("{}_{}\n", var.Info()->Name(), ver);

    PrintFormatted("FIRST = {{ ");
    for (const auto& s : var.FirstSet()) {
      PrintFormatted("{} ", s->Info()->Name());
    }
    if (var.MayProduceEpsilon()) {
      PrintFormatted("$epsilon ");
    }
    PrintFormatted("}}\n");

    PrintFormatted("FOLLOW = {{ ");
    for (const auto& s : var.FollowSet()) {
      PrintFormatted("{} ", s->Info()->Name());
    }
    if (var.MayProduceEpsilon()) {
      PrintFormatted("$eof ");
    }
    PrintFormatted("}}\n");
  }
}

auto PrintLexerAutomaton(const MetaInfo& /*info*/, const LexerAutomaton& dfa)
    -> void {
  PrintFormatted("[Lexing Automaton]\n");

  for (int id = 0; id < dfa.StateCount(); ++id) {
    const auto* const state = dfa.LookupState(id);
    PrintFormatted(
        "state {}({}):\n", state->id,
        state->acc_token ? state->acc_token->Name() : "NOT ACCEPTED");

    for (const auto& edge : state->transitions) {
      PrintFormatted("  {} -> {}\n", EscapeCharacter(edge.first),
                     edge.second->id);
    }

    PrintFormatted("\n");
  }
}

auto PrintParserAutomaton(const MetaInfo& info, const ParserAutomaton& pda)
    -> void {
  PrintFormatted("[Parsing Automaton]\n");

  for (int id = 0; id < pda.StateCount(); ++id) {
    const auto& state = *pda.LookupState(id);

    PrintFormatted("state {}:\n", id);

    if (state.EofAction()) {
      PrintFormatted("  <eof> -> do {}\n",
                     ToStringParserAction(info, *state.EofAction()));
    }

    for (const auto& pair : state.ActionMap()) {
      PrintFormatted("  {} -> do {}\n", pair.first->Name(),
                     ToStringParserAction(info, pair.second));
    }

    for (const auto& pair : state.GotoMap()) {
      PrintFormatted("  {} -> goto state {}\n", pair.first->Name(),
                     pair.second->Id());
    }

    PrintFormatted("\n");
  }
}

}  // namespace RG