#include "RegGen/Common/Configuration.h"

#include "RegGen/Common/Error.h"
#include "RegGen/Common/Format.h"
#include "RegGen/Common/Text.h"

namespace RG {

auto TryParseConstant(const char*& s, const char* text, bool skip_ws = true)
    -> bool {
  SkipWhitespace(s, skip_ws);
  return ConsumeIf(s, text);
}

auto ParseConstant(const char*& s, const char* text, bool skip_ws = true)
    -> void {
  SkipWhitespace(s, skip_ws);

  if (!ConsumeIf(s, text)) {
    throw ConfigParsingError{s, Format("expecting {}", text)};
  }
}

auto ParseIdentifier(const char*& s, bool skip_ws = true) -> std::string {
  SkipWhitespace(s, skip_ws);

  if (!isalpha(*s)) {
    throw ConfigParsingError{s, "expecting <identifier>"};
  }

  std::string buf;
  while (isalnum(*s) || *s == '_') {
    buf.push_back(Consume(s));
  }

  return buf;
}

auto ParseString(const char*& s, bool skip_ws = true) -> std::string {
  SkipWhitespace(s, skip_ws);

  if (!ConsumeIf(s, '"')) {
    throw ConfigParsingError{s, "expecting <string>"};
  }

  std::string buf{'"'};
  while (*s) {
    if (ConsumeIf(s, '"')) {
      if (ConsumeIf(s, '"')) {
        buf.push_back('"');
      } else {
        buf.push_back('"');
        return buf;
      }
    } else {
      buf.push_back(Consume(s));
    }
  }

  throw ConfigParsingError{s, "unexpected <eof>"};
}

auto ParseTypeSpec(const char*& s) -> QualType {
  auto type = ParseIdentifier(s);
  std::string qual;
  if (TryParseConstant(s, "'", false)) {
    qual = ParseIdentifier(s, false);
  }

  return QualType{std::move(type), std::move(qual)};
}

auto ParseTokenDefinition(ParserConfiguration& config, const char*& s,
                          bool ignore) -> void {
  auto name = ParseIdentifier(s);
  ParseConstant(s, "=");
  auto regex = ParseString(s);
  ParseConstant(s, ";");

  auto& target_vec = ignore ? config.ignored_tokens : config.tokens;
  target_vec.push_back(TokenDefinition{std::move(name), std::move(regex)});
}

auto ParseEnumDefinition(ParserConfiguration& config, const char*& s) -> void {
  auto name = ParseIdentifier(s);
  ParseConstant(s, "{");

  SmallVector<std::string> values;
  while (!TryParseConstant(s, "}")) {
    values.push_back(ParseIdentifier(s));
    ParseConstant(s, ";");
  }

  config.enums.push_back(EnumDefinition{std::move(name), std::move(values)});
}

auto ParseBaseDefinition(ParserConfiguration& config, const char*& s) -> void {
  auto name = ParseIdentifier(s);
  ParseConstant(s, ";");

  config.bases.push_back(BaseDefinition{std::move(name)});
}

auto ParseNodeDefinition(ParserConfiguration& config, const char*& s) -> void {
  auto name = ParseIdentifier(s);
  std::string parent;
  if (TryParseConstant(s, ":")) {
    parent = ParseIdentifier(s);
  }

  ParseConstant(s, "{");

  SmallVector<NodeMember> members;
  while (!TryParseConstant(s, "}")) {
    auto type = ParseTypeSpec(s);

    auto field_name = ParseIdentifier(s);
    ParseConstant(s, ";");

    members.push_back(NodeMember{type, field_name});
  }

  config.nodes.push_back(
      NodeDefinition{std::move(name), std::move(parent), std::move(members)});
}

auto ParseRuleDefinition(ParserConfiguration& config, const char*& s) -> void {
  auto name = ParseIdentifier(s);
  ParseConstant(s, ":");
  auto type = ParseTypeSpec(s);

  SmallVector<RuleItem> items;
  while (true) {
    ParseConstant(s, "=");

    SmallVector<RuleSymbol> rhs;

    SkipWhitespace(s);
    while (isalpha(*s) || *s == '"') {
      std::string symbol;
      if (*s == '"') {
        symbol = ParseString(s);
      } else {
        symbol = ParseIdentifier(s);
      }

      std::string assign;
      if (TryParseConstant(s, "!")) {
        assign = "!";
      } else if (TryParseConstant(s, "&")) {
        assign = "&";
      } else if (TryParseConstant(s, ":")) {
        assign = ParseIdentifier(s);
      }

      rhs.push_back(RuleSymbol{symbol, assign});

      SkipWhitespace(s);
    }

    std::optional<QualType> klass_hint;
    if (TryParseConstant(s, "->")) {
      if (TryParseConstant(s, "_")) {
        klass_hint = QualType{"_", ""};
      } else {
        klass_hint = ParseTypeSpec(s);
      }
    }

    items.push_back(RuleItem{std::move(rhs), std::move(klass_hint)});

    if (TryParseConstant(s, ";")) {
      break;
    }
  }

  config.rules.push_back(
      RuleDefinition{std::move(type), std::move(name), std::move(items)});
}

void ParseConfigInternal(ParserConfiguration& config, const char*& s) {
  while (*s) {
    if (TryParseConstant(s, "token")) {
      ParseTokenDefinition(config, s, false);
    } else if (TryParseConstant(s, "ignore")) {
      ParseTokenDefinition(config, s, true);
    } else if (TryParseConstant(s, "enum")) {
      ParseEnumDefinition(config, s);
    } else if (TryParseConstant(s, "base")) {
      ParseBaseDefinition(config, s);
    } else if (TryParseConstant(s, "node")) {
      ParseNodeDefinition(config, s);
    } else if (TryParseConstant(s, "rule")) {
      ParseRuleDefinition(config, s);
    } else {
      throw ConfigParsingError{s, "unexpected token"};
    }

    SkipWhitespace(s);
  }
}

auto ParseConfig(const std::string& data)
    -> std::unique_ptr<ParserConfiguration> {
  auto result = std::make_unique<ParserConfiguration>();

  try {
    const auto* p = data.c_str();
    ParseConfigInternal(*result, p);
  } catch (const ConfigParsingError& err) {
    auto around_text = std::string{err.pos}.substr(0, 20);
    throw ParserConstructionError{
        Format("LoadConfig: Failed parsing config file:{} at around \"{}\".",
               err.what(), around_text)};
  }

  return result;
}

}  // namespace RG