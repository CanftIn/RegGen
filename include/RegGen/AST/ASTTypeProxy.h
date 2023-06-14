#ifndef REGGEX_AST_AST_TYPE_PROXY_H
#define REGGEX_AST_AST_TYPE_PROXY_H

#include <type_traits>
#include <unordered_map>

#include "RegGen/AST/ASTBasic.h"
#include "RegGen/AST/ASTItem.h"
#include "RegGen/AST/ASTTypeTraits.h"
#include "RegGen/Common/Arena.h"
#include "RegGen/Common/Error.h"

namespace RG::AST {

class ASTTypeProxy {
 public:
  virtual auto ConstructEnum(int value) const -> ASTItem = 0;
  virtual auto ConstructObject(Arena&) const -> ASTItem = 0;
  virtual auto ConstructVector(Arena&) const -> ASTItem = 0;
  virtual auto ConstructOptional() const -> ASTItem = 0;

  virtual auto AssignField(ASTItem obj, int condition, ASTItem value) const
      -> void = 0;
  virtual auto PushBackElement(ASTItem vec, ASTItem elem) const -> void = 0;
};

class DummyASTTypeProxy : public ASTTypeProxy {
 private:
  [[noreturn]] static void Throw() {
    throw ParserInternalError{
        "DummyASTTypeProxy: Cannot perform any proxy operation."};
  }

 public:
  auto ConstructEnum(int /*value*/) const -> ASTItem override { Throw(); }
  auto ConstructObject(Arena&) const -> ASTItem override { Throw(); }
  auto ConstructVector(Arena&) const -> ASTItem override { Throw(); }
  auto ConstructOptional() const -> ASTItem override { Throw(); }

  auto AssignField(ASTItem /*obj*/, int /*condition*/, ASTItem /*value*/) const
      -> void override {
    Throw();
  }
  auto PushBackElement(ASTItem /*vec*/, ASTItem /*elem*/) const
      -> void override {
    Throw();
  }

  static auto Instance() -> const ASTTypeProxy& {
    static DummyASTTypeProxy dummy{};
    return dummy;
  }
};

}  // namespace RG::AST

#endif  // REGGEX_AST_AST_TYPE_PROXY_H