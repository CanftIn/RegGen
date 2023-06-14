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

static constexpr auto MsgUnexpectedEof = "Regex: Unexpected eof.";
static constexpr auto MsgEmptyExpressionBody =
    "Regex: Empty expression body is not allowed.";
static constexpr auto MsgInvalidClosure =
    "Regex: Invalid closure is not allowed.";

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

auto ParseCharClass(const char*& str) -> RegexExprPtr {
  const auto* p = str;
  bool reverse = ConsumeIf(p, '^');

  std::optional<int> last_ch;
  std::vector<CharRange> ranges{};

  // 0. parse character ranges
  while (*p && *p != ']') {
    if (last_ch) {
      // if a mark for range
      if (ConsumeIf(p, '-')) {
        RegexParsingAssert(!IsEof(p), MsgUnexpectedEof);

        auto ch = *last_ch;
        last_ch = std::nullopt;

        if (*p == ']') {
          // if '-' is the last character in char class
          // it's treated as a raw character
          ranges.push_back(CharRange{'-'});
          break;
        } else {
          int min = ch;
          int max = ParseCharacter(p);
          if (max < min) {
            std::swap(min, max);
          }

          ranges.push_back(CharRange{min, max});
        }
      } else {
        ranges.push_back(CharRange{*last_ch});
        last_ch = ParseCharacter(p);
      }
    } else {
      last_ch = ParseCharacter(p);
    }
  }

  if (last_ch) {
    ranges.push_back(CharRange{*last_ch});
  }

  RegexParsingAssert(ConsumeIf(p, ']'), MsgUnexpectedEof);
  RegexParsingAssert(!ranges.empty(), MsgEmptyExpressionBody);

  str = p;
  std::sort(ranges.begin(), ranges.end(),
            [](auto lhs, auto rhs) { return lhs.Min() < rhs.Min(); });

  // 1. merge ranges
  std::vector<CharRange> merged_ranges{ranges[0]};
  for (auto rg : ranges) {
    auto last_rg = merged_ranges.back();
    if (rg.Min() > last_rg.Min()) {
      merged_ranges.push_back(rg);
    } else {
      auto new_min = last_rg.Min();
      auto new_max = std::max(rg.Max(), last_rg.Max());

      merged_ranges.back() = CharRange{new_min, new_max};
    }
  }

  // 2. reverse ranges
  if (reverse) {
    std::vector<CharRange> reversed_ranges{};
    if (merged_ranges.front().Min() > 0) {
      reversed_ranges.push_back(CharRange{0, merged_ranges.front().Min() - 1});
    }

    for (auto it = merged_ranges.begin(); it != merged_ranges.end(); ++it) {
      auto next_it = std::next(it);
      if (next_it != merged_ranges.end()) {
        auto new_min = it->Max() + 1;
        auto new_max = next_it->Min() - 1;

        if (new_min <= new_max) {
          reversed_ranges.push_back(CharRange{new_min, new_max});
        }
      } else {
        if (it->Max() < 127) {
          reversed_ranges.push_back(CharRange{it->Max() + 1, 127});
        }
      }
    }

    merged_ranges = reversed_ranges;
  }
  RegexExprVec result;
  for (auto rg : merged_ranges) {
    result.push_back(std::make_unique<EntityExpr>(rg));
  }

  return result.size() == 1 ? std::move(result.front())
                            : std::make_unique<ChoiceExpr>(std::move(result));
}

auto ParseRegexInternal(const char*& str, char term) -> RegexExprPtr {
  RegexExprVec any{};
  RegexExprVec seq{};

  bool allow_closure = false;
  const auto* p = str;
  for (; *p && *p != term;) {
    if (ConsumeIf(p, '|')) {
      RegexParsingAssert(!seq.empty(), MsgEmptyExpressionBody);

      allow_closure = false;
      MergeSequence(any, seq);
    } else if (ConsumeIf(p, '(')) {
      allow_closure = true;

      seq.push_back(ParseRegexInternal(p, ')'));
    } else if (*p == '*' || *p == '+' || *p == '?') {
      RegexParsingAssert(!seq.empty(), MsgEmptyExpressionBody);
      RegexParsingAssert(allow_closure, MsgInvalidClosure);

      allow_closure = false;

      RepetitionMode strategy;
      switch (Consume(p)) {
        case '?':
          strategy = RepetitionMode::Optional;
          break;
        case '*':
          strategy = RepetitionMode::Star;
          break;
        case '+':
          strategy = RepetitionMode::Plus;
          break;
      }
      auto to_repeat = std::move(seq.back());
      seq.pop_back();

      seq.push_back(
          std::make_unique<ClosureExpr>(std::move(to_repeat), strategy));
    } else if (ConsumeIf(p, '[')) {
      allow_closure = true;

      seq.push_back(ParseCharClass(p));
    } else if (ConsumeIf(p, '\\')) {
      allow_closure = true;

      seq.push_back(ParseEscapedExpr(p));
    } else {
      allow_closure = true;

      seq.push_back(std::make_unique<EntityExpr>(CharRange{Consume(p)}));
    }
  }

  RegexParsingAssert(!seq.empty(), MsgEmptyExpressionBody);
  RegexParsingAssert(!term || ConsumeIf(p, term), MsgUnexpectedEof);

  str = p;
  MergeSequence(any, seq);

  return any.size() == 1 ? std::move(any[0])
                         : std::make_unique<ChoiceExpr>(std::move(any));
}

auto ParseRegex(const std::string& regex) -> std::unique_ptr<RootExpr> {
  const char* str = regex.c_str();
  return std::make_unique<RootExpr>(ParseRegexInternal(str, '\0'));
}

}  // namespace RG