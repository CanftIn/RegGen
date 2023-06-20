#ifndef DRIVER_HEADER_H
#define DRIVER_HEADER_H

#include "RegGen/RegGenInclude.h"

namespace RG {

using RG::BasicParser;
using RG::AST::ASTOptional;
using RG::AST::ASTTypeProxyManager;
using RG::AST::ASTVector;
using RG::AST::BasicASTEnum;
using RG::AST::BasicASTObject;
using RG::AST::BasicASTToken;
using RG::AST::BasicASTTypeProxy;
using RG::AST::DataBundle;

class Literal;
class Type;
class Expression;
class Statement;

class BoolLiteral;
class IntLiteral;
class NamedType;
class BinaryExpr;
class NamedExpr;
class LiteralExpr;
class VariableDeclStmt;
class JumpStmt;
class ReturnStmt;
class CompoundStmt;
class WhileStmt;
class ChoiceStmt;
class TypedName;
class FuncDecl;
class TranslationUnit;

enum BoolValue : int {
  True,
  False,
};

enum BinaryOp : int {
  Asterisk,
  Slash,
  Modulus,
  Plus,
  Minus,
  And,
  Or,
  Xor,
  Gt,
  GtEq,
  Ls,
  LsEq,
  Eq,
  NotEq,
  LogicAnd,
  LogicOr,
};

enum JumpCommand : int {
  Break,
  Continue,
};

enum VariableMutability : int {
  Val,
  Var,
};

class Literal : public BasicASTObject {
 public:
  struct Visitor {
    virtual void Visit(BoolLiteral&) = 0;
    virtual void Visit(IntLiteral&) = 0;
  };

  virtual void Accept(Visitor&) = 0;
};

class Type : public BasicASTObject {
 public:
  struct Visitor {
    virtual void Visit(NamedType&) = 0;
  };

  virtual void Accept(Visitor&) = 0;
};

class Expression : public BasicASTObject {
 public:
  struct Visitor {
    virtual void Visit(BinaryExpr&) = 0;
    virtual void Visit(NamedExpr&) = 0;
    virtual void Visit(LiteralExpr&) = 0;
  };

  virtual void Accept(Visitor&) = 0;
};

class Statement : public BasicASTObject {
 public:
  struct Visitor {
    virtual void Visit(VariableDeclStmt&) = 0;
    virtual void Visit(JumpStmt&) = 0;
    virtual void Visit(ReturnStmt&) = 0;
    virtual void Visit(CompoundStmt&) = 0;
    virtual void Visit(WhileStmt&) = 0;
    virtual void Visit(ChoiceStmt&) = 0;
  };

  virtual void Accept(Visitor&) = 0;
};

class BoolLiteral : public Literal, public DataBundle<BasicASTEnum<BoolValue>> {
 public:
  auto content() const -> const auto& { return GetItem<0>(); }

  void Accept(Literal::Visitor& v) override { v.Visit(*this); }
};

class IntLiteral : public Literal, public DataBundle<BasicASTToken> {
 public:
  auto content() const -> const auto& { return GetItem<0>(); }

  void Accept(Literal::Visitor& v) override { v.Visit(*this); }
};

class NamedType : public Type, public DataBundle<BasicASTToken> {
 public:
  auto name() const -> const auto& { return GetItem<0>(); }

  void Accept(Type::Visitor& v) override { v.Visit(*this); }
};

class BinaryExpr
    : public Expression,
      public DataBundle<BasicASTEnum<BinaryOp>, Expression*, Expression*> {
 public:
  auto op() const -> const auto& { return GetItem<0>(); }
  auto lhs() const -> const auto& { return GetItem<1>(); }
  auto rhs() const -> const auto& { return GetItem<2>(); }

  void Accept(Expression::Visitor& v) override { v.Visit(*this); }
};

class NamedExpr : public Expression, public DataBundle<BasicASTToken> {
 public:
  auto id() const -> const auto& { return GetItem<0>(); }

  void Accept(Expression::Visitor& v) override { v.Visit(*this); }
};

class LiteralExpr : public Expression, public DataBundle<Literal*> {
 public:
  auto content() const -> const auto& { return GetItem<0>(); }

  void Accept(Expression::Visitor& v) override { v.Visit(*this); }
};

class VariableDeclStmt : public Statement,
                         public DataBundle<BasicASTEnum<VariableMutability>,
                                           BasicASTToken, Type*, Expression*> {
 public:
  auto mut() const -> const auto& { return GetItem<0>(); }
  auto name() const -> const auto& { return GetItem<1>(); }
  auto type() const -> const auto& { return GetItem<2>(); }
  auto value() const -> const auto& { return GetItem<3>(); }

  void Accept(Statement::Visitor& v) override { v.Visit(*this); }
};

class JumpStmt : public Statement,
                 public DataBundle<BasicASTEnum<JumpCommand>> {
 public:
  auto command() const -> const auto& { return GetItem<0>(); }

  void Accept(Statement::Visitor& v) override { v.Visit(*this); }
};

class ReturnStmt : public Statement, public DataBundle<Expression*> {
 public:
  auto expr() const -> const auto& { return GetItem<0>(); }

  void Accept(Statement::Visitor& v) override { v.Visit(*this); }
};

class CompoundStmt : public Statement,
                     public DataBundle<ASTVector<Statement*>*> {
 public:
  auto children() const -> const auto& { return GetItem<0>(); }

  void Accept(Statement::Visitor& v) override { v.Visit(*this); }
};

class WhileStmt : public Statement, public DataBundle<Expression*, Statement*> {
 public:
  auto pred() const -> const auto& { return GetItem<0>(); }
  auto body() const -> const auto& { return GetItem<1>(); }

  void Accept(Statement::Visitor& v) override { v.Visit(*this); }
};

class ChoiceStmt
    : public Statement,
      public DataBundle<Expression*, Statement*, ASTOptional<Statement*>> {
 public:
  auto pred() const -> const auto& { return GetItem<0>(); }
  auto positive() const -> const auto& { return GetItem<1>(); }
  auto negative() const -> const auto& { return GetItem<2>(); }

  void Accept(Statement::Visitor& v) override { v.Visit(*this); }
};

class TypedName : public BasicASTObject,
                  public DataBundle<BasicASTToken, Type*> {
 public:
  auto name() const -> const auto& { return GetItem<0>(); }
  auto type() const -> const auto& { return GetItem<1>(); }
};

class FuncDecl : public BasicASTObject,
                 public DataBundle<BasicASTToken, ASTVector<TypedName*>*, Type*,
                                   ASTVector<Statement*>*> {
 public:
  auto name() const -> const auto& { return GetItem<0>(); }
  auto params() const -> const auto& { return GetItem<1>(); }
  auto ret() const -> const auto& { return GetItem<2>(); }
  auto body() const -> const auto& { return GetItem<3>(); }
};

class TranslationUnit : public BasicASTObject,
                        public DataBundle<ASTVector<FuncDecl*>*> {
 public:
  auto functions() const -> const auto& { return GetItem<0>(); }
};

inline auto CreateParser() -> BasicParser<TranslationUnit>::Ptr {
  static const auto* const config =
      u8R"##########(

# ===================================================
# Symbols
#

token s_assign = "=";
token s_semi = ";";
token s_colon = ":";
token s_arrow = "->";
token s_comma = ",";

token s_asterisk = "\*";
token s_slash = "/";
token s_modulus = "%";
token s_plus = "\+";
token s_minus = "-";
token s_amp = "&";
token s_bar = "\|";
token s_caret = "^";

token s_gt = ">";
token s_gteq = ">=";
token s_ls = "<";
token s_lseq = "<=";
token s_eq = "==";
token s_ne = "!=";

token s_ampamp = "&&";
token s_barbar = "\|\|";

token s_lp = "\(";
token s_rp = "\)";
token s_lb = "{";
token s_rb = "}";

# ===================================================
# Keywords
#

token k_func = "func";
token k_val = "val";
token k_var = "var";
token k_if = "if";
token k_else = "else";
token k_while = "while";
token k_break = "break";
token k_continue = "continue";
token k_return = "return";

token k_true = "true";
token k_false = "false";

token k_unit = "unit";
token k_int = "int";
token k_bool = "bool";

# ===================================================
# Component
#
token id = "[_a-zA-Z][_a-zA-Z0-9]*";
token l_int = "[0-9]+";

# ===================================================
# Ignore
#

ignore whitespace = "[ \t\r\n]+";

# ===================================================
# Literal
#

base Literal;

enum BoolValue
{ True; False; }

node BoolLiteral : Literal
{ BoolValue content; }

node IntLiteral : Literal
{ token content; }

rule BoolValue : BoolValue
    = k_true -> True
    = k_false -> False
    ;

rule BoolLiteral : BoolLiteral
    = BoolValue:content -> _
    ;

rule IntLiteral : IntLiteral
    = l_int:content -> _
    ;

# ===================================================
# Type
#

base Type;

node NamedType : Type
{
    token name;
}

rule KeywordNamedType : NamedType
    = k_unit:name -> _
    = k_bool:name -> _
    = k_int:name -> _
    ;
rule UserNamedType : NamedType
    = id:name -> _
    ;

rule Type : Type
    = KeywordNamedType!
    = UserNamedType!
    ;

# ===================================================
# Expression
#

# Operator enums
enum BinaryOp
{
    # multiplicative
    Asterisk; Slash; Modulus;

    # additive
    Plus; Minus;

    # bitwise op
    And; Or; Xor;

    # comparative
    Gt; GtEq; Ls; LsEq; Eq; NotEq;

    # logic composition
    LogicAnd; LogicOr;
}

rule MultiplicativeOp : BinaryOp
    = s_asterisk -> Asterisk
    = s_slash -> Slash
    = s_modulus -> Modulus
    ;
rule AdditiveOp : BinaryOp
    = s_plus -> Plus
    = s_minus -> Minus
    ;
rule BitwiseManipOp : BinaryOp
    = s_amp -> And
    = s_bar -> Or
    = s_caret -> Xor
    ;
rule ComparativeOp : BinaryOp
    = s_gt -> Gt
    = s_gteq -> GtEq
    = s_ls -> Ls
    = s_lseq -> LsEq
    = s_eq -> Eq
    = s_ne -> NotEq
    ;
rule LogicCompositionOp : BinaryOp
    = s_ampamp -> LogicAnd
    = s_barbar -> LogicOr
    ;

# Expression
base Expression;

node BinaryExpr : Expression
{
    BinaryOp op;
    Expression lhs;
    Expression rhs;
}
node NamedExpr : Expression
{
    token id;
}
node LiteralExpr : Expression
{
    Literal content;
}

rule Factor : Expression
    = IntLiteral:content -> LiteralExpr
    = BoolLiteral:content -> LiteralExpr
    = id:id -> NamedExpr
    = s_lp Expr! s_rp
    ;
rule MultiplicativeExpr : BinaryExpr
    = MultiplicativeExpr:lhs MultiplicativeOp:op Factor:rhs -> _
    = Factor!
    ;
rule AdditiveExpr : BinaryExpr
    = AdditiveExpr:lhs AdditiveOp:op MultiplicativeExpr:rhs -> _
    = MultiplicativeExpr!
    ;
rule BitwiseManipExpr : BinaryExpr
    = BitwiseManipExpr:lhs BitwiseManipOp:op AdditiveExpr:rhs -> _
    = AdditiveExpr!
    ;
rule ComparativeExpr : BinaryExpr
    = ComparativeExpr:lhs ComparativeOp:op BitwiseManipExpr:rhs -> _
    = BitwiseManipExpr!
    ;
rule LogicCompositionExpr : BinaryExpr
    = LogicCompositionExpr:lhs LogicCompositionOp:op ComparativeExpr:rhs -> _
    = ComparativeExpr!
    ;

rule Expr : Expression
    = LogicCompositionExpr!
    ;

# ===================================================
# Statement
#

# Helper enums
enum JumpCommand
{
    Break; Continue;
}
rule JumpCommand : JumpCommand
    = k_break -> Break
    = k_continue -> Continue
    ;

enum VariableMutability
{
    Val; Var;
}
rule VariableMutability : VariableMutability
    = k_val -> Val
    = k_var -> Var
    ;

# Decl
base Statement;

node VariableDeclStmt : Statement
{
    VariableMutability mut;
    token name;
    Type type;
    Expression value;
}
rule VariableDeclStmt : VariableDeclStmt
    = VariableMutability:mut id:name s_colon Type:type s_assign Expr:value s_semi -> _
    ;

node JumpStmt : Statement
{
    JumpCommand command;
}
rule JumpStmt : JumpStmt
    = JumpCommand:command s_semi -> _
    ;

node ReturnStmt : Statement
{
    Expression expr;
}
rule ReturnStmt : ReturnStmt
    = k_return Expr:expr s_semi -> _
    = k_return s_semi -> _
    ;

node CompoundStmt : Statement
{
    Statement'vec children;
}
rule StmtList : Statement'vec
    = Stmt& -> _
    = StmtList! Stmt&
    ;
rule StmtListInBrace : Statement'vec
    = s_lb s_rb -> _
    = s_lb StmtList! s_rb
    ;
rule CompoundStmt : CompoundStmt
    = StmtListInBrace:children -> _
    ;

# an AtomicStmt has absolutely no dangling else problem to solve
rule AtomicStmt : Statement
    = VariableDeclStmt!
    = JumpStmt!
    = ReturnStmt!
    = CompoundStmt!
    ;

node WhileStmt : Statement
{
    Expression pred;
    Statement body;
}
rule OpenWhileStmt : WhileStmt
    = k_while s_lp Expr:pred s_rp OpenStmt:body -> _
    ;
rule CloseWhileStmt : WhileStmt
    = k_while s_lp Expr:pred s_rp CloseStmt:body -> _
    ;

node ChoiceStmt : Statement
{
    Expression pred;
    Statement positive;
    Statement'opt negative;
}
rule OpenChoiceStmt : ChoiceStmt
    = k_if s_lp Expr:pred s_rp Stmt:positive -> ChoiceStmt
    = k_if s_lp Expr:pred s_rp CloseStmt:positive k_else OpenStmt:negative -> _
    ;
rule CloseChoiceStmt : ChoiceStmt
    = k_if s_lp Expr:pred s_rp CloseStmt:positive k_else CloseStmt:negative -> _
    ;

# OpenStmt is a statement contains at least one unpaired ChoiceStmt
rule OpenStmt : Statement
    = OpenWhileStmt!
    = OpenChoiceStmt!
    ;
# CloseStmt is a statement inside of which all ChoiceStmt are paired with an else
rule CloseStmt : Statement
    = AtomicStmt!
    = CloseWhileStmt!
    = CloseChoiceStmt!
    ;

rule Stmt : Statement
    = OpenStmt!
    = CloseStmt!
    ;

# ===================================================
# Top-level Declarations
#

node TypedName
{
    token name;
    Type type;
}
rule TypedName : TypedName
    = id:name s_colon Type:type -> _
    ;

node FuncDecl
{
    token name;

    TypedName'vec params;
    Type ret;

    Statement'vec body;
}
rule TypedNameList : TypedName'vec
    = TypedName& -> _
    = TypedNameList! s_comma TypedName&
    ;
rule FuncParameters : TypedName'vec
    = s_lp s_rp -> _
    = s_lp TypedNameList! s_rp
    ;
rule FuncDecl : FuncDecl
    = k_func id:name FuncParameters:params s_arrow Type:ret StmtListInBrace:body -> _
    ;

# ===================================================
# Global Symbol
#
node TranslationUnit
{
    FuncDecl'vec functions;
}

rule FuncDeclList : FuncDecl'vec
    = FuncDecl& -> _
    = FuncDeclList! FuncDecl&
    ;
rule TranslationUnit : TranslationUnit
    = FuncDeclList:functions -> _
    ;
)##########";
  static const auto proxy_manager = []() {
    ASTTypeProxyManager env;

    // register enums
    env.RegisterEnum<BoolValue>("BoolValue");
    env.RegisterEnum<BinaryOp>("BinaryOp");
    env.RegisterEnum<JumpCommand>("JumpCommand");
    env.RegisterEnum<VariableMutability>("VariableMutability");

    // register bases
    env.RegisterClass<Literal>("Literal");
    env.RegisterClass<Type>("Type");
    env.RegisterClass<Expression>("Expression");
    env.RegisterClass<Statement>("Statement");

    // register classes
    env.RegisterClass<BoolLiteral>("BoolLiteral");
    env.RegisterClass<IntLiteral>("IntLiteral");
    env.RegisterClass<NamedType>("NamedType");
    env.RegisterClass<BinaryExpr>("BinaryExpr");
    env.RegisterClass<NamedExpr>("NamedExpr");
    env.RegisterClass<LiteralExpr>("LiteralExpr");
    env.RegisterClass<VariableDeclStmt>("VariableDeclStmt");
    env.RegisterClass<JumpStmt>("JumpStmt");
    env.RegisterClass<ReturnStmt>("ReturnStmt");
    env.RegisterClass<CompoundStmt>("CompoundStmt");
    env.RegisterClass<WhileStmt>("WhileStmt");
    env.RegisterClass<ChoiceStmt>("ChoiceStmt");
    env.RegisterClass<TypedName>("TypedName");
    env.RegisterClass<FuncDecl>("FuncDecl");
    env.RegisterClass<TranslationUnit>("TranslationUnit");

    return env;
  }();

  return BasicParser<TranslationUnit>::Create(config, &proxy_manager);
}

}  // namespace RG

#endif  // DRIVER_HEADER_H