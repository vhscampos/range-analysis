/*
 * RADeadCodeElimination.h
 *
 *  Created on: Mar 21, 2013
 *      Author: raphael
 */

#ifndef OVERFLOWSANITIZER_H_
#define OVERFLOWSANITIZER_H_


//===- OverflowDetect.cpp - Example code from "Writing an LLVM Pass" ---===//
//
// This file implements the LLVM "Range Analysis Instrumentation" pass
//
//===----------------------------------------------------------------------===//
#include "llvm/Attributes.h"
#include "llvm/Type.h"
#include "llvm/Constants.h"
#include "llvm/DebugInfo.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Function.h"
#include "llvm/InstrTypes.h"
#include "llvm/Instructions.h"
#include "llvm/Instruction.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/Operator.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/Local.h"

#include <vector>
#include <set>
#include <list>
#include <stdio.h>

//#include "../RangeAnalysis/RangeAnalysis.h"
#include "../../Analysis/DepGraph/InputValues.h"
#include "../../Analysis/DepGraph/DepGraph.h"
#include "../uSSA/uSSA.h"

using namespace llvm;

namespace llvm{

	typedef enum { OvUnknown, OvCanHappen, OvWillHappen, OvWillNotHappen } OvfPrediction;

	struct OverflowSanitizer : public ModulePass {
	private:
		Module* module;
		std::map<Loop*, BasicBlock*> loopHeaders;
		std::map<BasicBlock*, Loop*> loopBlocks;
		std::set<BasicBlock*> vulnerableLoops;

		std::map<BasicBlock*, LoopInfo*> loopExitBlocks; // Maps a Loop Exit Block to its loop
		std::list<Value*> inpDepLoopConditions;
		std::set<Instruction*> valuesToSafe;
		std::set<Value*> inputValues;

		llvm::DenseMap<Function*, BasicBlock*> abortBlocks;


        llvm::LLVMContext* context;
        Constant* constZero;

        Value* GVstderr, *FPrintF, *overflowMessagePtr, *truncErrorMessagePtr, *overflowMessagePtr2, *truncErrorMessagePtr2;

        // Pointer to abort function
        Function *AbortF;

        std::map<std::string,Constant*> SourceFiles;

        void MarkAsNotOriginal(Instruction& inst);
        bool IsNotOriginal(Instruction& inst);
        static bool isValidInst(Instruction *I);

		BasicBlock* NewOverflowOccurrenceBlock(Instruction* I, BasicBlock* NextBlock, Value *messagePtr);

		void insertInstrumentation(Instruction* I, BasicBlock* AbortBB, OvfPrediction Pred);
		Constant* getSourceFile(Instruction* I);
		Constant* getLineNumber(Instruction* I);

		Instruction* GetNextInstruction(Instruction& i);

		void lookForValuesToSafe(Instruction* I, BasicBlock* loopHeader);

		Constant* strToLLVMConstant(std::string s);

		int countInstructions();
		int countOverflowableInsts();

		bool isReachable(BasicBlock* src, BasicBlock* dst);

		void analyzeLoops();
		void InsertGlobalDeclarations();

	public:
		static char ID;
		OverflowSanitizer() : ModulePass(ID), module(NULL), context(NULL),
							  constZero(NULL), GVstderr(NULL), FPrintF(NULL), overflowMessagePtr(NULL),
							  truncErrorMessagePtr(NULL), overflowMessagePtr2(NULL),
							  truncErrorMessagePtr2(NULL), AbortF(NULL){};

		virtual bool runOnModule(Module &M);

		virtual void getAnalysisUsage(AnalysisUsage &AU) const {
			AU.addRequired<moduleDepGraph>();
			AU.addRequired<InputValues>();
			AU.addRequired<LoopInfo>();
		}




	};
}
#endif /* OVERFLOWSANITIZER_H_ */
