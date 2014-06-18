/*
 * TripCountProfiler.h
 *
 *  Created on: Dec 10, 2013
 *      Author: raphael
 */

#ifndef TripCountProfiler_H_
#define TripCountProfiler_H_

#include "LoopInfoEx.h"
#include "ExitInfo.h"
#include "LoopControllersDepGraph.h"
#include "LoopNormalizerAnalysis.h"
#include "TripCountAnalysis.h"
#include "llvm/Pass.h"
#include "llvm/IR/IRBuilder.h"
#include <inttypes.h>
#include <map>
#include <set>
#include <stack>
#include <string>

namespace llvm {

	class TripCountProfiler: public FunctionPass {
	public:
		static char ID;

		Value* formatStr;
		Value* moduleIdentifierStr;
		llvm::LLVMContext* context;

		Value* initLoopList;
		Value* collectLoopData;
		Value* flushLoopStats;

		TripCountProfiler(): FunctionPass(ID),
				             formatStr(NULL), moduleIdentifierStr(NULL),
				             context(NULL), initLoopList(NULL), collectLoopData(NULL), flushLoopStats(NULL) {};
		~TripCountProfiler(){};

		virtual void getAnalysisUsage(AnalysisUsage &AU) const{

			AU.addRequired<ExitInfo>();

			AU.addRequired<LoopInfoEx>();

			AU.addRequired<LoopControllersDepGraph>();
			AU.addRequired<LoopNormalizerAnalysis>();

			AU.addRequired<TripCountAnalysis>();

		}

		Constant* strToLLVMConstant(std::string s);
		virtual bool doInitialization(Module &M);

		bool runOnFunction(Function &F);

		Value* generateEstimatedTripCount(BasicBlock* header, BasicBlock* entryBlock, Value* Op1, Value* Op2, CmpInst* CI);
		void saveTripCount(std::set<BasicBlock*> BB, AllocaInst* tripCountPtr, Value* estimatedTripCount,  BasicBlock* loopHeader, int LoopClass);

		Value* getValueAtEntryPoint(Value* source, BasicBlock* loopHeader);

		BasicBlock* findLoopControllerBlock(Loop* l);

	};

}

#endif
