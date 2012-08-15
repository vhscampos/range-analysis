/*
 *	This pass creates new definitions for variables used as operands
 *	of specifific instructions: add, sub, mul, trunc.
 *
 *	These new definitions are inserted right after the use site, and
 *	all remaining uses dominated by this new definition are renamed
 *	properly.
*/


#include "llvm/Pass.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/CFG.h"
#include "llvm/Instructions.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include "llvm/Constants.h"
#include <deque>
#include <algorithm>

namespace llvm {

class uSSA : public FunctionPass {
public:
	static char ID; // Pass identification, replacement for typeid.
	uSSA() : FunctionPass(ID) {}
	void getAnalysisUsage(AnalysisUsage &AU) const;
	bool runOnFunction(Function&);
	
	void createNewDefs(BasicBlock *BB);
	void renameNewDefs(Instruction *newdef);

private:
	// Variables always live
	DominatorTree *DT_;
	DominanceFrontier *DF_;
};

}

bool getTruncInstrumentation();
