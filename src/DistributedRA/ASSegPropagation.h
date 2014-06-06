/*
 * ASSegPropagation.h
 *
 *  Created on: Jun 5, 2014
 *      Author: raphael
 */

#ifndef ASSEGPROPAGATION_H_
#define ASSEGPROPAGATION_H_

#include "llvm/Pass.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Module.h"
#include "AliasSetSegmentation.h"

namespace llvm {

class ASSegPropagation: public llvm::ModulePass {
public:
	static char ID;

	ASSegPropagation(): ModulePass(ID) {}
	virtual ~ASSegPropagation() {}

	void getAnalysisUsage(AnalysisUsage &AU) const {
		AU.setPreservesAll();
		AU.addRequired<AliasSetSegmentation>();
	}

	bool runOnModule(Module &M);

	Range getRange(Value* Pointer);

};

} /* namespace llvm */


#endif /* ASSEGPROPAGATION_H_ */
