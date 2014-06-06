/*
 * AliasSetSegmentation.h
 *
 *  Created on: Jun 5, 2014
 *      Author: raphael
 */

#ifndef ALIASSETSEGMENTATION_H_
#define ALIASSETSEGMENTATION_H_

#include "llvm/Pass.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Module.h"
#include "RangedAliasTables.h"

namespace llvm {

class AliasSetSegmentation: public llvm::ModulePass {
public:
	static char ID;

	AliasSetSegmentation(): ModulePass(ID) {}
	virtual ~AliasSetSegmentation() {}

	void getAnalysisUsage(AnalysisUsage &AU) const {
		AU.setPreservesAll();
		AU.addRequired<RangedAliasTables>();
	}

	bool runOnModule(Module &M);

	RangedAliasTable* getTable(int AliasSetID);

};

} /* namespace llvm */

#endif /* ALIASSETSEGMENTATION_H_ */
