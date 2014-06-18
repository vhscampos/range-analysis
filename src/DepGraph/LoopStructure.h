/*
 * LoopStructure.h
 *
 *  Created on: Jan 10, 2014
 *      Author: raphael
 */

#ifndef LOOPSTRUCTURE_H_
#define LOOPSTRUCTURE_H_

#include "llvm/Pass.h"
#include "DepGraph.h"
#include "LoopInfoEx.h"
#include "LoopControllersDepGraph.h"
#include "LoopNormalizerAnalysis.h"
#include <set>
#include <stack>

using namespace std;

namespace llvm {

	class LoopStructure: public llvm::FunctionPass {
	public:
		static char ID;

		LoopStructure(): FunctionPass(ID) {}
		virtual ~LoopStructure() {}

		virtual void getAnalysisUsage(AnalysisUsage &AU) const{
			AU.addRequired<LoopControllersDepGraph>();
			AU.addRequiredTransitive<LoopInfoEx>();
			AU.addRequired<LoopNormalizerAnalysis>();
			AU.setPreservesAll();
		}

		bool doInitialization(Module& M);
		bool isIntervalComparison(ICmpInst* CI);
		bool runOnFunction(Function & F);
	};
}
#endif /* LOOPSTRUCTURE_H_ */
