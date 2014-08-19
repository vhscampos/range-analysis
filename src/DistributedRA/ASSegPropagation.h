/*
 * ASSegPropagation.h
 *
 *  Created on: Jun 5, 2014
 *      Author: raphael
 */

#ifndef ASSEGPROPAGATION_H_
#define ASSEGPROPAGATION_H_

#ifndef DEBUG_TYPE
#define DEBUG_TYPE "ASSegPropagation"
#endif

#include "llvm/Pass.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Module.h"
#include "AliasSetSegmentation.h"

#include "../NetDepGraph/DepGraph.h"
//#include "../../../NetDepGraph/src/NetDepGraph/InputDep.h"

namespace llvm {

class ASSegPropagation: public llvm::ModulePass {
public:
	static char ID;

	ASSegPropagation(): ModulePass(ID) {
		//as = 0;
	}
	virtual ~ASSegPropagation() {}

	std::map<int, SegmentationTable*> memToTableP1;
	std::map<int, SegmentationTable*> memToTableP2;
	std::map<int, SegmentationTable*> memToTable;


	void getAnalysisUsage(AnalysisUsage &AU) const {
		AU.setPreservesAll();
		AU.addRequired<AliasSets>();
		AU.addRequired<AliasSetSegmentation>();
		AU.addRequired<netDepGraph>();
		//AU.addRequired<InputDep>();
	}

	bool runOnModule(Module &M);

	Range getRange(Value* Pointer);

private:


	bool isSend(GraphNode *node);

	bool isRecv(GraphNode *node);

	bool isNodeP1(GraphNode *node);

};

} /* namespace llvm */


#endif /* ASSEGPROPAGATION_H_ */
