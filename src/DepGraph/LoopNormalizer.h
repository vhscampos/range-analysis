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
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"


namespace llvm {

	class LoopNormalizer: public llvm::FunctionPass {
	public:
		static char ID;

		LoopNormalizer(): FunctionPass(ID){};
		virtual ~LoopNormalizer() {};

		virtual void getAnalysisUsage(AnalysisUsage &AU) const{
			AU.addRequired<LoopInfoEx>();
		}

		virtual bool doInitialization(Module &M);

		bool runOnFunction(Function& F);

		void normalizePostExit(BasicBlock* exitBlock, BasicBlock* postExitBlock);

		BasicBlock* normalizePreHeaders(std::set<BasicBlock*> PreHeaders, BasicBlock* Header);
		void switchBranch(BasicBlock* FromBlock, BasicBlock* PreviousTo, BasicBlock* NewTo);

	};



}
#endif /* LOOPNORMALIZER_H_ */
