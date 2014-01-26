/*
 * SRATest.cpp
 *
 *  Created on: Nov 21, 2013
 *      Author: raphael
 */

#ifndef DEBUG_TYPE
#define DEBUG_TYPE "SRATest"
#endif

#include "llvm/Pass.h"
#include "SymbolicRangeAnalysis.h"
#include "Expr.h"
#include "Range.h"

using namespace llvm;

namespace llvm {

	class SRATest: public ModulePass {

	public:
		static char ID;

		SRATest(): ModulePass(ID){};

		bool runOnModule(Module &M){

			SymbolicRangeAnalysis& sra = getAnalysis<SymbolicRangeAnalysis>();

			for(Module::iterator Fit = M.begin(); Fit != M.end(); Fit++){

				for (Function::iterator BBit = Fit->begin(); BBit != Fit->end(); BBit++){

					for (BasicBlock::iterator Iit = BBit->begin(); Iit != BBit->end(); Iit++) {

						Instruction* I = Iit;

						Range R = sra.getRange(I);

						errs() << *I << "	" << R.getLower() << "	" << R.getUpper() << "\n";

					}
				}
			}



			return false;
		};

		void getAnalysisUsage(AnalysisUsage &AU) const {
			AU.addRequired<SymbolicRangeAnalysis> ();
			AU.setPreservesAll();
		}



	};

}

char SRATest::ID = 0;
static RegisterPass<SRATest> X("sra-printer",
		"Symbolic Range Analysis Printer");











