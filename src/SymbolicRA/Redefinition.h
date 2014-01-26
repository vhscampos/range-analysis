#ifndef _REDEFINITION_H_
#define _REDEFINITION_H_

#include "llvm/Pass.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include "llvm/IR/Instructions.h"

namespace llvm {

class Redefinition : public FunctionPass {
public:
  static char ID;
  Redefinition()
    : FunctionPass(ID), StatNumCreatedSigmas_(0),
      StatNumCreatedFrontierPhis_(0), StatNumInstructions_(0) { }

  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual bool runOnFunction(Function &F);

  virtual bool doFinalization(Module& M);

  static StringRef GetRedefPrefix() { return "redef"; }
  static StringRef GetPhiPrefix()   { return "phi";   }

private:
  void createSigmasInFunction(Function *F);
  void createSigmasForCondBranch(BranchInst *BI);
  void createSigmaNodesForValueAt(Value *V, Value *P, BasicBlock *BB);
  void createSigmaNodeForValueAt(Value *V, BasicBlock *BB,
                                 BasicBlock::iterator Position); 

  PHINode *createPhiNodeAt(Value *V, BasicBlock *BB);

  bool dominatesUse(Value *V, BasicBlock *BB);
  void replaceUsesOfWithAfter(Value *V, Value *R, BasicBlock *BB);

  DominatorTree *DT_;
  DominanceFrontier *DF_;

  // Statistics.
  unsigned StatNumCreatedSigmas_;
  unsigned StatNumCreatedFrontierPhis_;
  unsigned StatNumInstructions_;
};

}

#endif

