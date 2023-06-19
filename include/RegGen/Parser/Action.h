#ifndef REGGEN_PARSER_ACTION_H
#define REGGEN_PARSER_ACTION_H

#include "RegGen/Parser/TypeInfo.h"

namespace RG {

struct ActionError {};
struct ActionShift {
  int target_state;
};
struct ActionReduce {
  const ProductionInfo* production;
};

using ParserAction = std::variant<ActionError, ActionShift, ActionReduce>;

enum class ActionExecutionResult { Hungry, Consumed, Error };

}  // namespace RG

#endif  // REGGEN_PARSER_ACTION_H