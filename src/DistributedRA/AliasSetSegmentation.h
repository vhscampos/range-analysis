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
#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"
#include "SegmentationTables.h"
#include "../DepGraph/AliasSets.h"

namespace llvm {

class AliasSetSegmentation: public llvm::ModulePass {
public:
	static char ID;

	AliasSetSegmentation(): ModulePass(ID) {}
	virtual ~AliasSetSegmentation() {}

	void getAnalysisUsage(AnalysisUsage &AU) const {
		AU.setPreservesAll();
		AU.addRequired<AliasSets>();
		AU.addRequired<InterProceduralRA<Cousot> >();
		AU.addRequired<SegmentationTables>();
		
	}

	bool runOnModule(Module &M);

	SegmentationTable* getTable(int AliasSetID);
	
	SegmentationTable* UnionMerge (SegmentationTable* one, SegmentationTable* two);
	SegmentationTable* OffsetMerge (SegmentationTable* one, SegmentationTable* two, Range Offset);
	SegmentationTable* MultiplicationMerge (SegmentationTable* one, SegmentationTable* two);
	void ContentMerge (SegmentationTable* result, SegmentationTable* one, SegmentationTable* two);
	
private:
	llvm::DenseMap<int, SegmentationTable* > master_tables;
	llvm::DenseMap<int, std::set<SegmentationTable*> > tables_sets;
	
	bool CommonPointers(SegmentationTable* one, SegmentationTable* two);
	void BuildTable(SegmentationTable* result);
	SegmentationTable* OneLineTransformation (SegmentationTable* Table);
	
	bool intersection(Range i, Range j);
};

} /* namespace llvm */

#endif /* ALIASSETSEGMENTATION_H_ */
