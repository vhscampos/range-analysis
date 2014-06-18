/*
 * BranchAnalysis.h
 *
 *  Created on: Mar 21, 2014
 *      Author: raphael
 */

#ifndef BRANCHANALYSIS_H_
#define BRANCHANALYSIS_H_



//std includes
#include <map>
#include <list>

//LLVM includes
#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/ConstantRange.h"

//Our own stuff
#include "SymbolicInterval.h"
#include "ValueBranchMap.h"


using namespace std;

namespace llvm {

class BranchAnalysis: public llvm::FunctionPass {
private:
	std::map<const Value*, std::list<ValueSwitchMap*> > IntervalConstraints;

public:
	static char ID;
	BranchAnalysis(): FunctionPass(ID) {}
	virtual ~BranchAnalysis();

	static unsigned getMaxBitWidth(Module &M);
	static unsigned getMaxBitWidth(Function &F);
	static void updateMinMax(unsigned maxBitWidth);

	bool runOnFunction(Function &F);
	virtual bool doInitialization(Module &M);

	void buildValueSwitchMap(const SwitchInst *sw);
	void buildValueBranchMap(const BranchInst *br);

	void getAnalysisUsage(AnalysisUsage &AU) const{
		AU.setPreservesAll();
	}

	std::map<const Value*, std::list<ValueSwitchMap*> > getIntervalConstraints(){ return IntervalConstraints;};

};

} /* namespace llvm */

#endif /* BRANCHANALYSIS_H_ */
