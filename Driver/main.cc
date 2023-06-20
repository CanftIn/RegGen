#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>

#include "Header.h"
#include "RegGen/Common/Format.h"
#include "RegGen/RegGenInclude.h"

auto LoadConfigText() -> std::string {
  std::fstream file{"../lang_define.txt"};
  return std::string(std::istreambuf_iterator<char>{file}, {});
}

auto kConfig = LoadConfigText();
auto kSample = std::string{
    "func add(x: int, y: int) -> int { return x+y; }\n"
    "func mul(x: int, y: int) -> int { return x*y; }\n"
    "func main() -> unit { if(true) while(true) if(true) {} else {} else val "
    "x:int=41; }\n"};

auto ToStringToken(const RG::AST::BasicASTToken& tok) -> std::string {
  return kSample.substr(tok.Offset(), tok.Length());
}

auto ToStringType(RG::Type* type) -> std::string {
  const auto& t = reinterpret_cast<RG::NamedType*>(type);
  return ToStringToken(t->name());
}

auto ToStringExpr(RG::Expression* expr) -> std::string {
  return kSample.substr(expr->Offset(), expr->Length());
}

void PrintIdent(int ident) {
  for (int i = 0; i < ident; ++i) {
    putchar(' ');
  }
}

void PrintStatement(RG::Statement* s, int ident) {
  struct Visitor : RG::Statement::Visitor {
    int ident;

    void Visit(RG::VariableDeclStmt& stmt) override {
      PrintIdent(ident);

      const auto* mut =
          stmt.mut() == RG::VariableMutability::Val ? "mutable" : "immutable";
      RG::PrintFormatted("Variable Decl ({}) {} of {}\n", mut,
                         ToStringToken(stmt.name()), ToStringType(stmt.type()));
    }

    void Visit(RG::JumpStmt& stmt) override {
      PrintIdent(ident);

      if (stmt.command() == RG::JumpCommand::Break) {
        RG::PrintFormatted("Break\n");
      } else {
        RG::PrintFormatted("Continue\n");
      }
    }

    void Visit(RG::ReturnStmt& stmt) override {
      PrintIdent(ident);
      RG::PrintFormatted("Return {}\n", ToStringExpr(stmt.expr()));
    }

    void Visit(RG::CompoundStmt& stmt) override {
      PrintIdent(ident);
      if (stmt.children()->Empty()) {
        RG::PrintFormatted("Empty compound\n");
      } else {
        RG::PrintFormatted("Compound\n");
      }

      for (auto* child : stmt.children()->Value()) {
        PrintStatement(child, ident + 4);
      }
    }

    void Visit(RG::WhileStmt& stmt) override {
      PrintIdent(ident);
      RG::PrintFormatted("While {}\n", ToStringExpr(stmt.pred()));

      PrintStatement(stmt.body(), ident + 4);
    }

    void Visit(RG::ChoiceStmt& stmt) override {
      PrintIdent(ident);
      RG::PrintFormatted("Choice {}\n", ToStringExpr(stmt.pred()));

      PrintIdent(ident);
      RG::PrintFormatted("Positive:\n");
      PrintStatement(stmt.positive(), ident + 4);

      if (const auto& negative = stmt.negative(); negative.HasValue()) {
        PrintIdent(ident);
        RG::PrintFormatted("Negative:\n");
        PrintStatement(negative.Value(), ident + 4);
      }
    }
  };

  auto v = Visitor{};
  v.ident = ident;
  s->Accept(v);
}

void PrintFunctionDecl(RG::FuncDecl* f) {
  RG::PrintFormatted("Function {}@(offset:{}, length:{})\n",
                     ToStringToken(f->name()), f->Offset(), f->Length());

  RG::PrintFormatted("Parameters:\n");
  for (auto* param : f->params()->Value()) {
    RG::PrintFormatted("    {} of {}\n", ToStringToken(param->name()),
                       ToStringType(param->type()));
  }

  RG::PrintFormatted("Returns:\n");
  RG::PrintFormatted("    {}\n", ToStringType(f->ret()));

  RG::PrintFormatted("Body: [\n");
  for (auto* stmt : f->body()->Value()) {
    PrintStatement(stmt, 4);
  }

  RG::PrintFormatted("]\n");
}

void PrintTranslationUnit(const RG::TranslationUnit& u) {
  for (auto* f : u.functions()->Value()) {
    PrintFunctionDecl(f);
  }
}

auto main() -> int {
  // cout << BootstrapParser(kConfig);
  auto parser = RG::CreateParser();
  RG::Arena arena;
  auto* result = parser->Parse(arena, kSample);
}