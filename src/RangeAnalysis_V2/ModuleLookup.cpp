/*
 * ModuleLookup.cpp
 *
 *  Created on: Jun 15, 2014
 *      Author: raphael
 */

#include "ModuleLookup.h"

using namespace llvm;

bool llvm::ModuleLookup::runOnModule(Module& M) {

	module = &M;

	for (Module::iterator Fit = M.begin(), Fend = M.end(); Fit != Fend; ++Fit) {

		Function* F = Fit;

		for(Function::iterator BBit = Fit->begin(), BBend = Fit->end(); BBit != BBend; BBit++){

			for(BasicBlock::iterator Iit = BBit->begin(), Iend = BBit->end(); Iit != Iend; Iit++){

				Instruction* I = Iit;

				if(!I->getName().empty()) {

					FunctionLookup[F][I->getName()] = I;

				}
			}
		}
	}

	return false;
}


Value* llvm::ModuleLookup::getValueByName(std::string FunctionName,
		std::string ValueName) {

	Value* result = NULL;

	if (Function* F = module->getFunction(FunctionName)){

		if (FunctionLookup.count(F)){

			if (FunctionLookup[F].count(ValueName)){
				result = FunctionLookup[F][ValueName];
			}
		}
	}

	return result;
}


char ModuleLookup::ID = 0;
static RegisterPass<ModuleLookup> Y("module-lookup",
		"Module Lookup");
