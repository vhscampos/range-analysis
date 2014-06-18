/*
 * LoopNormalizer.h
 *
 *  Created on: Dec 10, 2013
 *      Author: raphael
 */

#ifndef LOOPNORMALIZER_H_
#define LOOPNORMALIZER_H_

#include "llvm/Pass.h"
#include "llvm/ADT/Statistic.h"
#include "LoopInfoEx.h"

namespace llvm {

	class LoopNormalizerAnalysis: public llvm::FunctionPass {
	public:
		static char ID;

		LoopNormalizerAnalysis(): FunctionPass(ID){};
		virtual ~LoopNormalizerAnalysis() {};

		virtual void getAnalysisUsage(AnalysisUsage &AU) const{
			AU.addRequiredTransitive<LoopInfoEx>();
			AU.setPreservesAll();
		}

		virtual bool doInitialization(Module &M);

		bool runOnFunction(Function& F);

		BasicBlock* getEntryBlock(Loop* L);
		BasicBlock* getEntryBlock(BasicBlock* LoopHeader);

		std::map<BasicBlock*, BasicBlock*> entryBlocks;

	};



}
#endif /* LOOPNORMALIZER_H_ */
