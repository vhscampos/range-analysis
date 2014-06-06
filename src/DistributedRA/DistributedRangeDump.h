/*
 * DistributedRangeDump.h
 *
 *  Created on: Jun 5, 2014
 *      Author: raphael
 */

#ifndef DISTRIBUTEDRANGEDUMP_H_
#define DISTRIBUTEDRANGEDUMP_H_

#include "llvm/Pass.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"

#include "../RangeAnalysis/RangeAnalysis.h"
#include "ASSegPropagation.h"

namespace llvm {

class DistributedRangeDump: public llvm::ModulePass {
private:
	std::string getOriginalFunctionName(Function* F);
public:
	static char ID;

	DistributedRangeDump(): ModulePass(ID) {}
	virtual ~DistributedRangeDump() {}

	void getAnalysisUsage(AnalysisUsage &AU) const {
		AU.setPreservesAll();
		//AU.addRequired<ASSegPropagation>();
	}

	bool runOnModule(Module &M);

};

}

#endif /* DISTRIBUTEDRANGEDUMP_H_ */
