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
#include "llvm/Analysis/Dominators.h"
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
#include "llvm/Transforms/Utils/Local.h"
#include "../uSSA/uSSA.h"
#include <vector>
#include <set>
#include <map>
#include <stdio.h>

#include "../RangeAnalysis/RangeAnalysis.h"

using namespace llvm;

namespace {

	struct RADeadCodeElimination : public FunctionPass {
		static char ID;
		RADeadCodeElimination() : FunctionPass(ID), function(NULL), context(NULL), constTrue(NULL), constFalse(NULL), ra(NULL) {};

		virtual bool runOnFunction(Function &F);


        bool solveICmpInstructions();
        bool solveICmpInstruction(ICmpInst* I);

        bool isValidBooleanOperation(Instruction* I);

        bool solveBooleanOperations();
        bool solveBooleanOperation(Instruction* I);

        void removeDeadInstructions();
        bool removeDeadCFGEdges();
        bool removeDeadBlocks();

        bool mergeBlocks();


        void replaceAllUses(Value* valueToReplace, Value* replaceWith);

        // Range Analysis stuff
        APInt Min, Max;

        bool isLimited(const Range &range) {
        	return range.isRegular() && range.getLower().ne(Min) && range.getUpper().ne(Max);
        }

        bool isConstantValue(Value* V);

		virtual void getAnalysisUsage(AnalysisUsage &AU) const {
			AU.addRequired<InterProceduralRA<Cousot> >();

			AU.addRequired<DominatorTree>();
		}


		Function* function;
        llvm::LLVMContext* context;
        Constant* constTrue;
        Constant* constFalse;
        InterProceduralRA<Cousot>* ra;

        std::set<Instruction*> deadInstructions;
        std::set<BasicBlock*> deadBlocks;

	};
}
#endif /* RADEADCODEELIMINATION_H_ */
