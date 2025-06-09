#include "llvm/ADT/APInt.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

struct ShiftMulPass : public PassInfoMixin<ShiftMulPass> {
  bool isPowerOfTwo(const APInt &Val) {
    return Val.getBoolValue() && !(Val & (Val - 1));
  }

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    bool Changed = false;

    for (BasicBlock &BB : F) {
      for (Instruction &I : make_early_inc_range(BB)) {
        if (auto *BinOp = dyn_cast<BinaryOperator>(&I)) {
          if (BinOp->getOpcode() != Instruction::Mul) continue;
          if (!BinOp->getType()->isIntegerTy()) continue;

          Value *LHS = BinOp->getOperand(0);
          Value *RHS = BinOp->getOperand(1);

          ConstantInt *LHSC = dyn_cast<ConstantInt>(LHS);
          ConstantInt *RHSC = dyn_cast<ConstantInt>(RHS);

          ConstantInt *PowerOfTwo = nullptr;
          Value *OtherOperand = nullptr;
          bool IsLeftOperand = false;

          if (LHSC && isPowerOfTwo(LHSC->getValue())) {
            PowerOfTwo = LHSC;
            OtherOperand = RHS;
            IsLeftOperand = true;
          } else if (RHSC && isPowerOfTwo(RHSC->getValue())) {
            PowerOfTwo = RHSC;
            OtherOperand = LHS;
            IsLeftOperand = false;
          }

          if (!PowerOfTwo) continue;

          APInt ShiftValue = PowerOfTwo->getValue();
          unsigned ShiftAmount = ShiftValue.exactLogBase2();

          IRBuilder<> Builder(BinOp);
          
          Constant *ShiftConst = ConstantInt::get(
              BinOp->getType(), ShiftAmount);

          Value *NewInst = Builder.CreateShl(OtherOperand, ShiftConst);

          BinOp->replaceAllUsesWith(NewInst);
          BinOp->eraseFromParent();
          Changed = true;

          errs() << "Replaced mul with shl in: " << F.getName() << "\n";
        }
      }
    }

    return Changed ? PreservedAnalyses::none() 
                  : PreservedAnalyses::all();
  }
};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {
      LLVM_PLUGIN_API_VERSION, 
      "ShiftMulPass", 
      "v0.1",
      [](PassBuilder &PB) {
        PB.registerPipelineParsingCallback(
            [](StringRef Name, FunctionPassManager &FPM,
               ArrayRef<PassBuilder::PipelineElement>) {
              if (Name == "shift-mul-pass") {
                FPM.addPass(ShiftMulPass());
                return true;
              }
              return false;
            });
      }};
}