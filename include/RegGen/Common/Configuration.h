#ifndef REGGEN_COMMON_CONFIGURATION_H
#define REGGEN_COMMON_CONFIGURATION_H

#include "RegGen/Common/Definition.h"

namespace RG {

struct ParserConfiguration {
  SmallVector<TokenDefinition> tokens;
  SmallVector<TokenDefinition> ignored_tokens;
  SmallVector<EnumDefinition> enums;
  SmallVector<NodeDefinition> nodes;
  SmallVector<RuleDefinition> rules;
  SmallVector<BaseDefinition> bases;
};

auto ParseConfig(const std::string& data)
    -> std::unique_ptr<ParserConfiguration>;

}  // namespace RG

#endif  // REGGEN_COMMON_CONFIGURATION_H