/*
 * TripCountAnalysis.h
 *
 *  Created on: Dec 10, 2013
 *      Author: raphael
 */

#ifndef TripCountAnalysis_H_
#define TripCountAnalysis_H_

#include "LoopInfoEx.h"
#include "LoopControllersDepGraph.h"
#include "LoopNormalizerAnalysis.h"
#include "llvm/Pass.h"
#include "llvm/IR/IRBuilder.h"
#include <inttypes.h>
#include <map>
#include <set>
#include <stack>
#include <string>

namespace llvm {

	class TripCountAnalysis: public FunctionPass {
	private:
		bool IsTripCount(Instruction& inst);
		std::map<BasicBlock*,Instruction*> tripCounts;
	public:
		static char ID;

		TripCountAnalysis(): FunctionPass(ID) {};
		~TripCountAnalysis(){};

		virtual void getAnalysisUsage(AnalysisUsage &AU) const{
			AU.addRequired<LoopInfoEx>();
			AU.addRequired<LoopNormalizerAnalysis>();
			AU.setPreservesAll();
		}

		bool runOnFunction(Function &F);

		Instruction* getTripCount(BasicBlock* loopHeader) {return tripCounts[loopHeader];};
	};

}

#endif
