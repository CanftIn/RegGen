#ifndef REGGEN_COMMON_DEFINITION_H
#define REGGEN_COMMON_DEFINITION_H

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>

#include "RegGen/Container/SmallVector.h"

namespace RG {

struct QualType {
  std::string name;
  std::string qual;
};

struct TokenDefinition {
  std::string id;
  std::string regex;
};

struct EnumDefinition {
  std::string name;
  SmallVector<std::string> choices;
};

struct BaseDefinition {
  std::string name;
};

struct NodeMember {
  QualType type;
  std::string name;
};

struct NodeDefinition {
  std::string name;
  std::string parent;

  SmallVector<NodeMember> members;
};

struct RuleSymbol {
  std::string symbol;
  std::string assign;
};

struct RuleItem {
  SmallVector<RuleSymbol> rhs;
  std::optional<QualType> class_hint;
};

struct RuleDefinition {
  QualType type;

  std::string name;
  SmallVector<RuleItem, 0> items;
};

}  // namespace RG

#endif  // REGGEN_COMMON_DEFINITION_H