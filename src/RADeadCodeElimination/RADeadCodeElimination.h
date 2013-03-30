/*
 * RADeadCodeElimination.h
 *
 *  Created on: Mar 21, 2013
 *      Author: raphael
 */

#ifndef RADEADCODEELIMINATION_H_
#define RADEADCODEELIMINATION_H_


//===- OverflowDetect.cpp - Example code from "Writing an LLVM Pass" ---===//
//
// This file implements the LLVM "Range Analysis Instrumentation" pass
//
//===----------------------------------------------------------------------===//
#include "llvm/Type.h"
#include "llvm/Constants.h"
#include "llvm/Operator.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Function.h"
#include "llvm/InstrTypes.h"
#include "llvm/Instructions.h"
#include "llvm/Instruction.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"
#include "../uSSA/uSSA.h"
#include <vector>
#include <set>
#include <map>
#include <stdio.h>

#include "../RangeAnalysis/RangeAnalysis.h"

using namespace llvm;

namespace {

	struct RADeadCodeElimination : public ModulePass {
		static char ID;
		RADeadCodeElimination() : ModulePass(ID), module(NULL), context(NULL), constTrue(NULL), constFalse(NULL), ra(NULL) {};

        virtual bool runOnModule(Module &M);
        void init(Module &M);



        bool solveICmpInstructions();
        bool solveICmpInstruction(ICmpInst* I);

        bool isValidBooleanOperation(Instruction* I);

        bool solveBooleanOperations();
        bool solveBooleanOperation(Instruction* I);

        void removeDeadInstructions();
        bool removeDeadCFGEdges();
        bool removeDeadBlocks();
        bool ConstantFoldTerminator(BasicBlock *BB);
        void setUnconditionalDest(BranchInst *BI, BasicBlock *Destination);


        void replaceAllUses(Value* valueToReplace, Value* replaceWith);

        // Range Analysis stuff
        APInt Min, Max;

        bool isLimited(const Range &range) {
        	return range.isRegular() && range.getLower().ne(Min) && range.getUpper().ne(Max);
        }

        bool isConstantValue(Value* V);

		virtual void getAnalysisUsage(AnalysisUsage &AU) const {
			AU.addRequiredTransitive<DominatorTree>();

			AU.addRequired<InterProceduralRA<Cousot> >();
		}


        Module* module;
        llvm::LLVMContext* context;
        Constant* constTrue;
        Constant* constFalse;
        InterProceduralRA<Cousot>* ra;

        std::set<Instruction*> deadInstructions;
        std::set<BasicBlock*> deadBlocks;

	};
}
#endif /* RADEADCODEELIMINATION_H_ */
