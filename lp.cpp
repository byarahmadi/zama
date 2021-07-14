#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/raw_ostream.h"

using namespace std;
using namespace llvm;

enum Token {
  tok_eof = -1,
  tok_or = 15,
  tok_and = 16,
  tok_identifier = -4,
  tok_not = -5,
  tok_number = -6
};

static std::string IdentifierStr;
static int NumVal;
static int gettok() {
  static int LastChar = ' ';

  // Skip any whitespace.
  while (isspace(LastChar)) LastChar = getchar();

  if (isalpha(LastChar)) {  // identifier: [a-zA-Z][a-zA-Z0-9]*
    IdentifierStr = LastChar;
    while (isalnum((LastChar = getchar()))) IdentifierStr += LastChar;
    if (IdentifierStr == "true" || IdentifierStr == "false") {
      NumVal = 0;
      if (IdentifierStr == "true") NumVal = 1;
      return tok_number;
    }
    if (IdentifierStr == "or") return tok_or;
    if (IdentifierStr == "and") return tok_and;
    if (IdentifierStr == "not") return tok_not;

    return tok_identifier;
  }

  // Check for end of file.  Don't eat the EOF.
  if (LastChar == EOF) return tok_eof;

  // Otherwise, just return the character as its ascii value.
  int ThisChar = LastChar;
  LastChar = getchar();
  return ThisChar;
}
/* template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
} */

namespace {

/// ExprAST - Base class for all expression nodes.
class ExprAST {
 public:
  virtual ~ExprAST() = default;
  virtual Value *codeGen() = 0;
};

class NumberExprAST : public ExprAST {
  int Val;

 public:
  NumberExprAST(int Val) : Val(Val) {}

  Value *codeGen() override;
};

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST : public ExprAST {
  std::string Name;

 public:
  VariableExprAST(const std::string &Name) : Name(Name) {}
  Value *codeGen() override;
};
class UnaryExprAST : public ExprAST {
  char Op;
  std::unique_ptr<ExprAST> Operand;

 public:
  UnaryExprAST(char Op, std::unique_ptr<ExprAST> Operand)
      : Op(Op), Operand(std::move(Operand)) {}
  Value *codeGen() override;
};

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST {
  char Op;
  std::unique_ptr<ExprAST> LHS, RHS;

 public:
  BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS,
                std::unique_ptr<ExprAST> RHS)
      : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
  Value *codeGen() override;
};

}  // end anonymous namespace

//===----------------------------------------------------------------------===//
// Parser
//===----------------------------------------------------------------------===//

/// CurTok/getNextToken - Provide a simple token buffer.  CurTok is the current
/// token the parser is looking at.  getNextToken reads another token from the
/// lexer and updates CurTok with its results.
static int CurTok;
static int getNextToken() { return CurTok = gettok(); }

/// BinopPrecedence - This holds the precedence for each binary operator that is
/// defined.
static std::map<int, int> BinopPrecedence;

/// GetTokPrecedence - Get the precedence of the pending binary operator token.
static int GetTokPrecedence() {
  if (!isascii(CurTok)) return -1;

  // Make sure it's a declared binop.
  int TokPrec = BinopPrecedence[CurTok];
  if (TokPrec <= 0) return -1;
  return TokPrec;
}

/// LogError* - These are little helper functions for error handling.
std::unique_ptr<ExprAST> LogError(const char *Str) {
  fprintf(stderr, "Error: %s\n", Str);
  return nullptr;
}
static std::unique_ptr<ExprAST> ParseExpression();

static std::unique_ptr<ExprAST> ParseNumberExpr() {
  // auto Result = llvm::make_unique<NumberExprAST>(NumVal);
  cout << "The numbers is " << IdentifierStr;
  getNextToken();  // consume the number
  return llvm::make_unique<NumberExprAST>(NumVal);
}
static std::unique_ptr<ExprAST> ParseUnaryExpr() {
  getNextToken();  // eat not
  if (CurTok != '(') return LogError("expected '('");
  getNextToken();  // eat (.
  auto V = ParseExpression();
  if (!V) return nullptr;

  if (CurTok != ')') return LogError("expected ')'");
  getNextToken();  // eat ).
  return llvm::make_unique<UnaryExprAST>(tok_not, std::move(V));
}

/// parenexpr ::= '(' expression ')'
static std::unique_ptr<ExprAST> ParseParenExpr() {
  getNextToken();  // eat (.
  auto V = ParseExpression();
  if (!V) return nullptr;

  if (CurTok != ')') return LogError("expected ')'");
  getNextToken();  // eat ).
  return V;
}

/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
static std::unique_ptr<ExprAST> ParseIdentifierExpr() {
  std::string IdName = IdentifierStr;

  getNextToken();  // eat identifier.
  cout << endl << "Identifier " << IdName << " has been found" << endl;

  return llvm::make_unique<VariableExprAST>(IdName);
}

/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
static std::unique_ptr<ExprAST> ParsePrimary() {
  switch (CurTok) {
    default:
      return LogError("unknown token when expecting an expression");
    case tok_identifier:
      return ParseIdentifierExpr();
    case tok_number:
      return ParseNumberExpr();
    case tok_not:
      return ParseUnaryExpr();
    case '(':
      return ParseParenExpr();
  }
}

/// binoprhs
///   ::= ('+' primary)*
static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
                                              std::unique_ptr<ExprAST> LHS) {
  // If this is a binop, find its precedence.
  while (true) {
    int TokPrec = GetTokPrecedence();

    cout << endl << "In a ParsBinOpRHS" << endl;

    cout << IdentifierStr << endl;

    // If this is a binop that binds at least as tightly as the current binop,
    // consume it, otherwise we are done.
    if (TokPrec < ExprPrec) return LHS;

    int BinOp = CurTok;
    getNextToken();  // eat binop

    auto RHS = ParsePrimary();
    if (!RHS) return nullptr;

    // If BinOp binds less tightly with RHS than the operator after RHS, let
    // the pending operator take RHS as its LHS.
    int NextPrec = GetTokPrecedence();
    if (TokPrec < NextPrec) {
      RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
      if (!RHS) return nullptr;
    }

    // Merge LHS/RHS.
    LHS =
        llvm::make_unique<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));
  }
}

/// expression
///   ::= primary binoprhs
///
static std::unique_ptr<ExprAST> ParseExpression() {
  auto LHS = ParsePrimary();
  if (!LHS) return nullptr;

  return ParseBinOpRHS(0, std::move(LHS));
}

//==------------------------------------------------------------------------===//
// Code generation
//===----------------------------------------------------------------------===//
static LLVMContext TheContext;
static IRBuilder<> Builder(TheContext);
static std::unique_ptr<Module> TheModule;
static std::map<std::string, Value *> NamedValues;

GlobalVariable *createGlob(IRBuilder<> &Builder, std::string Name) {
  TheModule->getOrInsertGlobal(Name, Builder.getInt1Ty());
  GlobalVariable *gVar = TheModule->getNamedGlobal(Name);
  gVar->setLinkage(GlobalValue::ExternalLinkage);
  gVar->setAlignment(1);
  return gVar;
}
Value *NumberExprAST::codeGen() { return Builder.getInt1(Val); }

Value *VariableExprAST::codeGen() {
  if (!NamedValues[Name]) {
    Value *v = createGlob(Builder, Name);
    NamedValues[Name] = v;
  }
  LoadInst *load = Builder.CreateLoad(NamedValues[Name]);

  return load;
}

Value *UnaryExprAST::codeGen() {
  Value *Opr = Operand->codeGen();
  return Builder.CreateNot(Opr, "nottmp");
}

Value *BinaryExprAST::codeGen() {
  Value *L = LHS->codeGen();
  Value *R = RHS->codeGen();
  if (!L || !R) return nullptr;

  L->dump();
  R->dump();
  switch (Op) {
    case tok_and:

      return Builder.CreateAnd(L, R, "andtmp");
      break;
    case tok_or:
      return Builder.CreateOr(L, R, "ortmp");
      break;
    default:
      return nullptr;
  }
}

//===----------------------------------------------------------------------===//
// Top-Level parsing
//===----------------------------------------------------------------------===//

static void HandleTopLevelExpression() {
  // Evaluate a top-level expression into an anonymous function.
  if (auto topExpr = ParseExpression()) {
    fprintf(stderr, "Parsed a top-level expr\n");
    topExpr->codeGen();
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}

/// top ::= definition | external | expression | ';'
static void MainLoop() {
  while (true) {
    fprintf(stderr, "ready> ");
    switch (CurTok) {
      case tok_eof:
        return;
      default:
        HandleTopLevelExpression();
        break;
    }
  }
}

//===----------------------------------------------------------------------===//
// Main driver code.
//===----------------------------------------------------------------------===//

int main() {
  // Install standard binary operators.
  // 1 is lowest precedence.
  BinopPrecedence[tok_or] = 10;
  BinopPrecedence[tok_and] = 20;
  // Prime the first token.
  fprintf(stderr, "ready> ");
  getNextToken();

  TheModule = llvm::make_unique<Module>("module_template", TheContext);
  FunctionType *funcType = llvm::FunctionType::get(Builder.getInt32Ty(), false);
  Function *fooFunc = llvm::Function::Create(
      funcType, llvm::Function::ExternalLinkage, "foo", &*TheModule);
  BasicBlock *entry = BasicBlock::Create(TheContext, "entry", fooFunc);
  Builder.SetInsertPoint(entry);

  // Run the main "interpreter loop" now.
  MainLoop();
  // The basic block terminator
  Builder.CreateRet(Builder.getInt32(0));
  TheModule->print(errs(), nullptr);

  verifyModule(*TheModule);

  std::error_code EC;
  llvm::raw_fd_ostream OS("IR.bc", EC, llvm::sys::fs::F_None);
  WriteBitcodeToFile(&*TheModule, OS);
  OS.flush();

  return 0;
}
