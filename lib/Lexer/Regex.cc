#include "RegGen/Lexer/Regex.h"

#include <optional>
#include <vector>

#include "RegGen/Common/Error.h"
#include "RegGen/Common/Text.h"

namespace RG {

auto RegexParsingAssert(bool condition, const std::string& msg) -> void {
  if (!condition) {
    throw ParserConstructionError(msg);
  }
}

static constexpr auto MsgUnexpectedEof = "Regex: Unexpected eof";
static constexpr auto MsgEmptyExpressionBody =
    "Regex: Empty expression body is not allowed";
static constexpr auto MsgInvalidClosure =
    "Regex: Invalid closure is not allowed";

auto MergeSequence(RegexExprVec& any, RegexExprVec& seq) -> void {
  assert(!seq.empty());

  auto reg_vec = RegexExprVec(seq.size());
  std::move(seq.begin(), seq.end(), reg_vec.begin());
  seq.clear();

  if (reg_vec.size() == 1) {
    any.push_back(std::move(reg_vec[0]));
  } else {
    any.push_back(std::make_unique<SequenceExpr>(std::move(reg_vec)));
  }
}

auto ParseCharacter(const char*& str) -> int {
  int result;
  const auto* p = str;
  if (ConsumeIf(p, '\\')) {
    RegexParsingAssert(!IsEof(p), MsgUnexpectedEof);

    result = EscapeRawCharacter(Consume(p));
  } else {
    result = static_cast<unsigned>(Consume(p));
  }
  str = p;
  return result;
}

auto ParseEscapedExpr(const char*& str) -> RegexExprPtr {
  auto rg = CharRange{EscapeRawCharacter(Consume(str))};
  return std::make_unique<EntityExpr>(rg);
}



}  // namespace RG