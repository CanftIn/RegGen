#ifndef REGGEN_COMMON_DEBUG_H
#define REGGEN_COMMON_DEBUG_H

#include "RegGen/Lexer/LexerAutomaton.h"
#include "RegGen/Parser/Grammar.h"
#include "RegGen/Parser/MetaInfo.h"
#include "RegGen/Parser/ParserAutomaton.h"

namespace RG {

auto ToStringToken(const MetaInfo& info, int id) -> std::string;
auto ToStringVariable(const MetaInfo& info, int id) -> std::string;
auto ToStringProduction(const ProductionInfo& p) -> std::string;

auto PrintMetaInfo(const MetaInfo& info) -> void;
auto PrintGrammar(const Grammar& g) -> void;

auto PrintLexerAutomaton(const MetaInfo& info, const LexerAutomaton& dfa)
    -> void;
auto PrintParserAutomaton(const MetaInfo& info, const ParserAutomaton& pda)
    -> void;

}  // namespace RG

#endif  // REGGEN_COMMON_DEBUG_H