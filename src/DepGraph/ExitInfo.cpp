/*
 * ExitInfo.cpp
 *
 *  Created on: Dec 16, 2013
 *      Author: raphael
 */

#include "ExitInfo.h"

using namespace llvm;

bool ExitInfo::runOnModule(Module &M) {

    // get pointer to function main
	Function* main = M.getFunction("main");

	// Using fortran? ... this kind of works
	if (!main)
		main = M.getFunction("MAIN__");


	// get pointer to function exit
	Function* exitF = M.getFunction("exit");

	FunctionType *AbortFTy = FunctionType::get(Type::getVoidTy(M.getContext()), false);
	Value* abortF = M.getOrInsertFunction("abort", AbortFTy);


	//Iterate through functions
	for(Module::iterator Fit = M.begin(); Fit != M.end(); Fit++){

		Function* F = Fit;

		bool isMain = (F == main);

		//Iterate through basic blocks
		for (Function::iterator BB = Fit->begin(); BB != Fit->end(); BB++){

			TerminatorInst* T = BB->getTerminator();

			if (dyn_cast<UnreachableInst>(T)){

				//scan BasicBlock looking for callInst to the exit function
				for(BasicBlock::iterator Iit = BB->begin(), Iend = BB->end(); Iit != Iend; Iit++){

					if(CallInst* CI = dyn_cast<CallInst>(Iit)){

						Function* calledfunction = CI->getCalledFunction();

						if(calledfunction == exitF || calledfunction == abortF)  exitPoints.insert(CI);

					}

				}

			} else if (ReturnInst* RI = dyn_cast<ReturnInst>(T)) {
				//May not be the exit point, when main is called recursively
				if (isMain) exitPoints.insert(RI);
			} else if (dyn_cast<InvokeInst>(T)) {
				//Invokes functions externally linked >> the control
				// may never come back to the program
			}

		}

	}

	return false;
}

char ExitInfo::ID = 0;
static RegisterPass<ExitInfo> Y("exit-info",
                "Exit Information");
