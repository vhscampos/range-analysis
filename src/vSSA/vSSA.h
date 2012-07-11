#include "llvm/Pass.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/CFG.h"
#include "llvm/Instructions.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include <deque>
#include <algorithm>

namespace llvm {

class vSSA : public FunctionPass {
public:
	static char ID; // Pass identification, replacement for typeid.
	vSSA() : FunctionPass(ID) {}
	void getAnalysisUsage(AnalysisUsage &AU) const;
	bool runOnFunction(Function&);

private:
	// Variables always live
	DominatorTree *DT_;
	DominanceFrontier *DF_;
	void createSigmasIfNeeded(BasicBlock *BB);
	void insertSigmas(TerminatorInst *TI, Value *V);
	void renameUsesToSigma(Value *V, PHINode *sigma);
	SmallVector<PHINode*, 25> insertPhisForSigma(Value *V, PHINode *sigma);
	void insertPhisForPhi(Value *V, PHINode *phi);
	void renameUsesToPhi(Value *V, PHINode *phi);
	void insertSigmaAsOperandOfPhis(SmallVector<PHINode*, 25> &vssaphi_created, PHINode *sigma);
	void populatePhis(SmallVector<PHINode*, 25> &vssaphi_created, Value *V);
	bool dominateAny(BasicBlock *BB, Value *value);
	bool dominateOrHasInFrontier(BasicBlock *BB, BasicBlock *BB_next, Value *value);
	bool verifySigmaExistance(Value *V, BasicBlock *BB, BasicBlock *from);
};

}
