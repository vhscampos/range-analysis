/*
 * RangeAnalysis.h
 *
 *  Created on: Mar 19, 2014
 *      Author: raphael
 */

#ifndef RANGEANALYSIS_H_
#define RANGEANALYSIS_H_

//std includes
#include <iostream>
#include <istream>
#include <fstream>
#include <list>
#include <map>

//LLVM includes
#include "llvm/Pass.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Support/CommandLine.h"


//Our own stuff
#include "../DepGraph/DepGraph.h"
#include "BranchAnalysis.h"
#include "ModuleLookup.h"
#include "Range.h"

#define MaxIterationCount 1

namespace llvm {

typedef enum {
        loJoin = 0, loMeet = 1
} LatticeOperation;

class RangeAnalysis {
private:
	std::map<SigmaOpNode*, BasicInterval*> branchConstraints;
	std::map<GraphNode*,Range> initial_state;
	std::map<GraphNode*,Range> out_state;
	std::map<GraphNode*,int> widening_count;
	std::map<GraphNode*,int> narrowing_count;
	std::set<std::string> ignoredFunctions;

	void fixPointIteration(int SCCid, LatticeOperation lo);
	void fixFutures(int SCCid);
	void computeNode(GraphNode* Node, std::set<GraphNode*> &Worklist, LatticeOperation lo);
	void addSuccessorsToWorklist(GraphNode* Node, std::set<GraphNode*> &Worklist);

	Range getInitialState(GraphNode* Node);
	Range evaluateNode(GraphNode* Node);
	Range getUnionOfPredecessors(GraphNode* Node);
	Range abstractInterpretation(Range Op1, Range Op2, Instruction *I);
	Range abstractInterpretation(Range Op1, Instruction *I);

	bool join(GraphNode* Node, Range new_abstract_state);
	bool meet(GraphNode* Node, Range new_abstract_state);

	void printSCCState(int SCCid);

protected:
	void solve();
	void addConstraints(std::map<const Value*, std::list<ValueSwitchMap*> > constraints);

	void importInitialStates(ModuleLookup& M);
	void loadIgnoredFunctions(std::string FileName);
	void addIgnoredFunction(std::string FunctionName);

	void computeStats();
public:
	DepGraph* depGraph;

	RangeAnalysis(): depGraph(NULL) {};
	virtual ~RangeAnalysis() {};

	Range getRange(Value* V);
};

} /* namespace llvm */



class IntraProceduralRA: public FunctionPass, public RangeAnalysis {
public:
	static char ID;
	IntraProceduralRA():FunctionPass(ID){}
	virtual ~IntraProceduralRA(){};

    void getAnalysisUsage(AnalysisUsage &AU) const;

    bool runOnFunction(Function &F);
};

class InterProceduralRA: public ModulePass, public RangeAnalysis {
public:
	static char ID;
	InterProceduralRA():ModulePass(ID){}
	virtual ~InterProceduralRA(){};

    void getAnalysisUsage(AnalysisUsage &AU) const;

    bool runOnModule(Module &M);
};




#endif /* RANGEANALYSIS_H_ */
