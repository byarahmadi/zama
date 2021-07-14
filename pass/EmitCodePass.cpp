#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
using namespace llvm;
using namespace std;
//
namespace {

class EmitCodePass : public BasicBlockPass {
 private:
  static void emitCode(std::string codeLine) {
    static std::ofstream file("output");
    // file.open("output");
    file << codeLine << endl;
    // file.close();
  }
  string regOneValue;
  string regZeroValue;
  static string getOperandName(Value *operand) {
    string operandName = operand->getName();
    if (operandName == "") {
      if (Constant *CI = dyn_cast<Constant>(operand)) {
        operandName = CI->isOneValue() ? "true" : "false";
      } else {
        operandName = "notDefined";
      }
    }
    return operandName;
  }
  static string getReg() {
    static int i = 0;
    string reg = "r" + to_string(i);
    i++;
    return reg;
  }
  static string emitCodeNot(string operand0) {
    string reg0 = getReg();
    emitCode(reg0 + ", " + operand0 + " nand " + operand0);
    string reg1 = getReg();
    emitCode(reg1 + ", " + reg0 + " nand " + reg0);
    return reg1;
  }
  static string emitCodeAnd(string operand0, string operand1) {
    string reg0 = getReg();
    emitCode(reg0 + ", " + operand0 + " nand " + operand1);
    string reg1 = getReg();
    emitCode(reg1 + ", " + reg0 + " nand " + reg0);
    return reg1;
  }
  static string emitCodeOr(string operand0, string operand1) {
    string reg0 = getReg();
    emitCode(reg0 + ", " + operand0 + " nand " + operand0);
    string reg1 = getReg();
    emitCode(reg1 + ", " + operand1 + " nand " + operand1);
    string reg2 = getReg();
    emitCode(reg2 + ", " + reg0 + " nand " + reg1);
    return reg2;
  }

 public:
  static char ID;
  EmitCodePass() : BasicBlockPass(ID) {}
  // Assumption : there is only one Basic Block in the CFG!!!
  virtual bool runOnBasicBlock(BasicBlock &BB) {
    unordered_map<Value *, string> defToRegMap;
    if (BB.size() <= 1) return false;
    regZeroValue = getReg();  // r0 is always equal to 0.
    regOneValue = getReg();   // r1 is always equal to 1.
    emitCode(regOneValue + ", " + regZeroValue + " nand " + regZeroValue);

    for (auto IIT = BB.begin(), IE = BB.end(); IIT != IE; ++IIT) {
      Instruction &Inst = *IIT;
      if (LoadInst *LI = dyn_cast<LoadInst>(&Inst)) {
        string reg = getReg();
        defToRegMap[LI] = reg;
      }
      Inst.dump();
      if (Inst.isBinaryOp()) {
        string opName = Inst.getOpcodeName();
        string operand0 = "";
        string operand1 = "";

        Value *v0 = Inst.getOperand(0);
        if (defToRegMap.find(v0) != defToRegMap.end())
          operand0 = defToRegMap[v0];

        Value *v1 = Inst.getOperand(1);
        if (defToRegMap.find(v1) != defToRegMap.end())
          operand1 = defToRegMap[v1];

        if (operand0 == "") {
          operand0 = getOperandName(Inst.getOperand(0));
          operand0 = operand0 == "true" ? regOneValue : regZeroValue;
        }
        if (operand1 == "") {
          operand1 = getOperandName(Inst.getOperand(1));
          operand1 = operand1 == "true" ? regOneValue : regZeroValue;
        }
        string defReg;
        // xor is not  =>  operand XOR true
        if (opName == "xor") {
          defReg = emitCodeNot(operand0);
        }
        if (opName == "and") {
          defReg = emitCodeAnd(operand0, operand1);
        }
        if (opName == "or") {
          defReg = emitCodeOr(operand0, operand1);
        }

        defToRegMap[&Inst] = defReg;
      }
    }

    return false;
  }
};
}  // namespace

char EmitCodePass::ID = 0;
static RegisterPass<EmitCodePass> X("emit-code-pass", "Emit code for zama arch",
                                    false, false);
