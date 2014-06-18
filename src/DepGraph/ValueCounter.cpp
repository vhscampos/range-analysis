/*
 * InputDeps.cpp
 *
 *  Created on: 05/04/2013
 *      Author: raphael
 */

#define DEBUG_TYPE "ValueCounter"

#include "ValueCounter.h"

using namespace llvm;

STATISTIC(TotalValues, "Number of values (of all kinds)");

void llvm::ValueCounter::getAnalysisUsage(AnalysisUsage& AU) const {

	/*
	 * We won't modify anything, so we shall tell this to LLVM
	 * to allow LLVM to not invalidate other analyses.
	 */
	AU.setPreservesAll();
}

bool llvm::ValueCounter::runOnModule(Module& M) {

	std::set<Value*> values;

	for(Module::iterator Fit = M.begin(), Fend = M.end(); Fit != Fend; Fit++){

		if (!values.count(Fit)) values.insert(Fit);

		for(Function::arg_iterator Arg = Fit->arg_begin(), aEnd = Fit->arg_end(); Arg != aEnd; Arg++) {
			if (!values.count(Arg)) values.insert(Arg);
		}

		for (Function::iterator BBit = Fit->begin(), BBend = Fit->end(); BBit != BBend; BBit++) {

			for (BasicBlock::iterator Iit = BBit->begin(), Iend = BBit->end(); Iit != Iend; Iit++) {

				if (!values.count(Iit)) values.insert(Iit);

				for(unsigned int i = 0; i < Iit->getNumOperands(); i++){

					if (!values.count(Iit->getOperand(i))) values.insert(Iit->getOperand(i));

				}
			}
		}
	}
	TotalValues = values.size();

	//We don't modify anything, so we must return false;
	return false;
}

char ValueCounter::ID = 0;
static RegisterPass<ValueCounter> Y("value-counter", "Value counter");


