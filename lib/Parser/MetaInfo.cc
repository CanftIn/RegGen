#include "RegGen/Parser/MetaInfo.h"

#include <algorithm>
#include <cassert>
#include <sstream>

#include "RegGen/Common/Configuration.h"
#include "RegGen/Common/TypeDefinition.h"
#include "RegGen/Parser/TypeInfo.h"

namespace RG {

class MetaInfo::Builder {
 public:
  auto Build(const std::string& config, const AST::ASTTypeProxyManager* env)
      -> std::unique_ptr<MetaInfo> {
    auto cc = ParseConfig(config);

    Initialize(config, env);

    LoadTypeInfo(*cc);
    LoadSymbolInfo(*cc);

    return Finalize();
  }

 private:
  auto Initialize(const std::string& /*config*/,
                  const AST::ASTTypeProxyManager* env) -> void {
    site_ = std::make_unique<MetaInfo>();
    site_->env_ = env;
  }

  auto Finalize() -> std::unique_ptr<MetaInfo> { return std::move(site_); }

  auto Assert(bool pred, const char* msg) -> void {
    if (!pred) {
      throw ParserConstructionError{msg};
    }
  }

  auto RegisterTypeInfo(TypeInfo* info) -> void {
    if (site_->type_lookup_.count(info->Name()) > 0) {
      throw ParserConstructionError{
          "ParsingMetaInfoBuilder: duplicate type name"};
    }

    site_->type_lookup_[info->Name()] = info;
  }

  auto RegisterSymbolInfo(SymbolInfo* info) -> void {
    if (site_->symbol_lookup_.count(info->Name()) > 0) {
      throw ParserConstructionError{
          "ParsingMetaInfoBuilder: duplicate symbol name"};
    }

    site_->symbol_lookup_[info->Name()] = info;
  }

  auto TranslateTypeSpec(const QualType& def) -> TypeSpec {
    using Qualifier = TypeSpec::Qualifier;

    auto qual = def.qual == "vec"   ? Qualifier::Vector
                : def.qual == "opt" ? Qualifier::Optional
                                    : Qualifier::None;

    auto* type = site_->type_lookup_.at(def.name);

    return TypeSpec{qual, type};
  }

  auto MakeTokenInfo(const TokenDefinition& def, int id) -> TokenInfo {
    auto result = TokenInfo(id, def.name);
    result.text_def_ = RemoveQuote(def.regex);
    result.ast_def_ = ParseRegex(result.text_def_);

    return result;
  }

  auto ConstructAstHandle(const TypeSpec& var_type, const RuleItem& rule)
      -> std::unique_ptr<AST::ASTHandle> {
    const auto& type_lookup = site_->type_lookup_;
    const auto& symbol_lookup = site_->symbol_lookup_;

    const auto is_vec = var_type.IsVector();
    const auto is_opt = var_type.IsOptional();
    const auto is_qualified = is_vec || is_opt;

    const auto is_enum = !is_vec && (var_type.type->IsEnum());
    const auto is_obj =
        !is_vec && (var_type.type->IsClass() || var_type.type->IsBase());

    auto* rule_type_info = var_type.type;

    auto gen_handle = [&]() -> AST::ASTHandle::GenHandle {
      if (rule.class_hint) {
        const auto& hint = *rule.class_hint;

        if (is_opt) {
          if (hint.name == "_" || hint.qual == "opt") {
            return AST::ASTOptionalGen{};
          }
        }

        if (is_enum) {
          auto* info = reinterpret_cast<EnumTypeInfo*>(var_type.type);
          auto value = distance(
              info->values_.begin(),
              find(info->values_.begin(), info->values_.end(), hint.name));

          Assert(value < info->values_.size(),
                 "ParserMetaInfo::Builder: invalid enum member.");
          return AST::ASTEnumGen{static_cast<int>(value)};
        } else {
          if (hint.name != "_") {
            rule_type_info = type_lookup.at(hint.name);
          }

          if (is_vec) {
            return AST::ASTVectorGen{};
          } else {
            assert(is_obj);
            return AST::ASTObjectGen{};
          }
        }
      } else {
        auto pred = [](const auto& elem) { return elem.assign == "!"; };
        const auto* it = std::find_if(rule.rhs.begin(), rule.rhs.end(), pred);
        auto index = static_cast<int>(std::distance(rule.rhs.begin(), it));

        Assert(it != rule.rhs.end(),
               "ParserMetaInfo::Builder: rule does not return.");
        Assert(
            std::find_if(std::next(it), rule.rhs.end(), pred) == rule.rhs.end(),
            "ParserMetaInfo::Builder: multiple item selected to return.");

        auto* symbol = symbol_lookup.at(it->symbol);
        if (symbol->IsVariable()) {
          rule_type_info = symbol->AsVariable()->type_.type;
        }

        return AST::ASTItemSelector{index};
      }
    }();

    auto manip_handle = [&]() -> AST::ASTHandle::ManipHandle {
      const auto& info = *dynamic_cast<ClassTypeInfo*>(rule_type_info);

      SmallVector<int> to_be_pushed;
      SmallVector<AST::ASTObjectSetter::SetterPair> to_be_assigned;

      for (int i = 0; i < rule.rhs.size(); ++i) {
        const auto& symbol = rule.rhs[i];

        if (symbol.assign == "&") {
          to_be_pushed.push_back(i);
        } else {
          if (!symbol.assign.empty() && symbol.assign != "!") {
            const auto* it =
                std::find_if(info.members_.begin(), info.members_.end(),
                             [&](const ClassTypeInfo::MemberInfo& mem) {
                               return mem.name == symbol.assign;
                             });

            Assert(it != info.members_.end(), "ParserMetaInfo::Builder:");

            auto conditional = std::distance(info.members_.begin(), it);
            to_be_assigned.push_back({static_cast<int>(conditional), i});
          }
        }
      }

      if (is_vec) {
        Assert(to_be_assigned.empty(),
               "ParserMetaInfo::Builder: unexpected opertion(assign).");

        if (to_be_pushed.empty()) {
          return AST::ASTManipPlaceholder{};
        } else {
          return AST::ASTVectorMerger{to_be_pushed};
        }
      } else if (is_obj) {
        Assert(to_be_pushed.empty(),
               "ParserMetaInfo::Builder: unexpected opertion(push).");

        if (to_be_assigned.empty()) {
          return AST::ASTManipPlaceholder{};
        } else {
          return AST::ASTObjectSetter{to_be_assigned};
        }
      } else {
        assert(is_opt || is_enum);

        Assert(to_be_pushed.empty() && to_be_assigned.empty(),
               "ParserMetaInfo::Builder: unexpected opertion(assign or push).");

        return AST::ASTManipPlaceholder{};
      }
    }();

    const auto& proxy = site_->env_
                            ? site_->env_->Lookup(rule_type_info->Name())
                            : &AST::DummyASTTypeProxy::Instance();

    return std::make_unique<AST::ASTHandle>(proxy, gen_handle,
                                            std::move(manip_handle));
  }

  auto LoadTypeInfo(const ParserConfiguration& config) -> void {
    auto& type_lookup = site_->type_lookup_;
    auto& enums = site_->enums_;
    auto& bases = site_->bases_;
    auto& classes = site_->classes_;

    type_lookup.insert_or_assign("token", &TokenTypeInfo::Instance());

    enums.initialize(config.enums.size());
    for (int i = 0; i < enums.size(); ++i) {
      const auto& def = config.enums[i];
      auto& info = enums[i];

      info = EnumTypeInfo{def.name};
      info.values_ = def.choices;

      RegisterTypeInfo(&info);
    }

    bases.initialize(config.bases.size());
    for (int i = 0; i < bases.size(); ++i) {
      const auto& def = config.bases[i];
      auto& info = bases[i];

      info = BaseTypeInfo{def.name};

      RegisterTypeInfo(&info);
    }

    classes.initialize(config.nodes.size());
    for (int i = 0; i < classes.size(); ++i) {
      const auto& def = config.nodes[i];
      auto& info = classes[i];

      info = ClassTypeInfo{def.name};

      RegisterTypeInfo(&info);
    }

    for (int i = 0; i < classes.size(); ++i) {
      const auto& def = config.nodes[i];
      auto& info = classes[i];

      if (!def.parent.empty()) {
        auto it = type_lookup.find(def.parent);
        Assert(it != type_lookup.end() && it->second->IsBase(),
               "ParserMetaInfo::Builder: invalid base type specified.");

        info.base_ = reinterpret_cast<BaseTypeInfo*>(it->second);
      }

      for (const auto& member_def : def.members) {
        info.members_.push_back(
            {TranslateTypeSpec(member_def.type), member_def.name});
      }
    }
  }

  auto LoadSymbolInfo(const ParserConfiguration& config) -> void {
    auto& symbol_lookup = site_->symbol_lookup_;
    auto& tokens = site_->tokens_;
    auto& ignored_tokens = site_->ignored_tokens_;
    auto& variables = site_->variables_;
    auto& productions = site_->productions_;

    tokens.initialize(config.tokens.size());
    for (int i = 0; i < tokens.size(); ++i) {
      const auto& def = config.tokens[i];
      auto& info = tokens[i];

      info = MakeTokenInfo(def, i);

      RegisterSymbolInfo(&info);
    }
    ignored_tokens.initialize(config.ignored_tokens.size());
    for (int i = 0; i < ignored_tokens.size(); ++i) {
      const auto& def = config.ignored_tokens[i];
      auto& info = ignored_tokens[i];

      info = MakeTokenInfo(def, i + tokens.size());
    }

    auto production_cnt = 0;
    variables.initialize(config.rules.size());
    for (int i = 0; i < variables.size(); ++i) {
      const auto& def = config.rules[i];
      auto& info = variables[i];

      info = VariableInfo(i, def.name);
      info.type_ = TranslateTypeSpec(def.type);

      production_cnt += def.items.size();
      RegisterSymbolInfo(&info);
    }

    productions.initialize(production_cnt);
    int pd_index = 0;
    for (const auto& rule_def : config.rules) {
      auto* lhs = dynamic_cast<VariableInfo*>(symbol_lookup.at(rule_def.name));

      for (const auto& rule_item : rule_def.items) {
        auto& info = productions[pd_index++];

        info.lhs_ = lhs;
        for (const auto& symbol_name : rule_item.rhs) {
          info.rhs_.push_back(symbol_lookup.at(symbol_name.symbol));
        }

        info.handle_ = ConstructAstHandle(lhs->type_, rule_item);

        // inject ProductionInfo back into VariableInfo
        lhs->productions_.push_back(&info);
      }
    }
  }

  std::unique_ptr<MetaInfo> site_;
};

auto ResolveParserInfo(const std::string& config,
                       const AST::ASTTypeProxyManager* env)
    -> std::unique_ptr<MetaInfo> {
  MetaInfo::Builder builder{};

  return builder.Build(config, env);
}

}  // namespace RG