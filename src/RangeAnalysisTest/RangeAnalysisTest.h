#ifndef LLVM_TRANSFORMS_RANGEANALYSIS_RANGEANALYSISTESTS_H_
#define LLVM_TRANSFORMS_RANGEANALYSIS_RANGEANALYSISTESTS_H_

#include "llvm/Function.h"
#include "llvm/Pass.h"
#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/ConstantRange.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/TimeValue.h"
#include "llvm/Support/Process.h"
#include "../RangeAnalysis/RangeAnalysis.h"

#define ASSERT_TRUE(print_op,op,op1,op2,eRes) {total++; \
			Range realRes = op1.op(op2); \
			if(realRes != eRes){ \
			failed++; \
			errs() << "\t[" << total << "] " << print_op << ": "; \
			op1.print(errs()); \
			errs() << " "; \
			op2.print(errs()); \
			errs() << " RESULT: "; \
			realRes.print(errs()); \
			errs() << " EXPECTED: "; \
			eRes.print(errs()); \
			errs() << "\n";}}

class RangeUnitTest: public ModulePass{
	unsigned total;
	unsigned failed;
	void printStats();
public:
	static char ID; // Pass identification, replacement for typeid
	RangeUnitTest() : ModulePass(ID), total(0), failed(0) {}
	bool runOnModule(Module & M);
};

#endif /* LLVM_TRANSFORMS_RANGEANALYSIS_RANGEANALYSISTESTS_H_ */
