/*
 * DepGraphSCCAnalysis.cpp
 *
 *  Created on: 04/11/2013
 *      Author: raphael
 */

#ifndef DEBUG_TYPE
#define DEBUG_TYPE "DepGraphSCCAnalysis"
#endif

#include "LoopControllersDepGraph.h"
#include <map>
#include <set>
#include <stack>
#include <string>

using namespace std;

namespace llvm {

//STATISTIC(NumNestedLoops, "Number of Nested Loops");
STATISTIC(MaxNumFunctionsInSCC, "Maximum number of functions in the same SCC");
STATISTIC(MaxSCCSize, "Maximum SCC size");
STATISTIC(NumInterproceduralSCCs, "Number of interprocedural SCCs");
STATISTIC(NumSCCs, "Number of SCCs");
STATISTIC(NumSCCWithTwoOrMoreNodes, "Number of SCCs with two or more nodes");

class DepGraphSCCAnalysis: public FunctionPass {
private:
	DepGraph *g;

public:
	static char ID; // Pass identification, replacement for typeid.

	DepGraphSCCAnalysis() :
		FunctionPass(ID), g(NULL) {
	}

	void getAnalysisUsage(AnalysisUsage &AU) const {

		AU.addRequired<LoopControllersDepGraph> ();
		AU.setPreservesAll();
	}

	bool doInitialization(Module &M){
		NumSCCWithTwoOrMoreNodes = 0;
		MaxNumFunctionsInSCC = 0;
		MaxSCCSize = 0;
		NumInterproceduralSCCs = 0;
		NumSCCs = 0;
		return false;
	}

	bool runOnFunction(Function& F) {

		LoopControllersDepGraph& DepGraphPass = getAnalysis<LoopControllersDepGraph> ();
		DepGraph *g = DepGraphPass.depGraph;

		std::map<int, std::set<GraphNode*> > SCCs = g->getSCCs();

		NumSCCs += SCCs.size();

		for(std::map<int, std::set<GraphNode*> >::iterator it = SCCs.begin(); it!=SCCs.end(); it++){

			unsigned int SCCSize =  it->second.size();

			if (MaxSCCSize < SCCSize) MaxSCCSize += it->second.size();

			if (SCCSize > 1) {
				NumSCCWithTwoOrMoreNodes++;
			}

			//Collect list of functions of the SCC
			std::set<Function*> SCCFunctions;
			for(std::set<GraphNode*>::iterator node = it->second.begin(); node != it->second.end(); node++) {
				GraphNode* N = *node;
				if (OpNode* On = dyn_cast<OpNode>(N)){
					if(Value* V = On->getOperation()){
						if(Instruction* I = dyn_cast<Instruction>(V)) {
							Function* F = I->getParent()->getParent();
							if(!SCCFunctions.count(F)) SCCFunctions.insert(F);
						}
					}
				}
				if (VarNode* Vn = dyn_cast<VarNode>(N)){
					if(Value* V = Vn->getValue()){
						if(Instruction* I = dyn_cast<Instruction>(V)) {
							Function* F = I->getParent()->getParent();
							if(!SCCFunctions.count(F)) SCCFunctions.insert(F);
						}
					}
				}
			}

			if (MaxNumFunctionsInSCC < SCCFunctions.size()) MaxNumFunctionsInSCC += SCCFunctions.size();

			if (SCCFunctions.size() > 1) NumInterproceduralSCCs++;

		}

		return false;
	}
};

char DepGraphSCCAnalysis::ID = 0;
static RegisterPass<DepGraphSCCAnalysis> X("depgraph-scc-analysis",
		"SCC Analysis of Dependence Graph");




class ModuleDepGraphSCCAnalysis: public ModulePass {
private:
	DepGraph *g;

public:
	static char ID; // Pass identification, replacement for typeid.

	ModuleDepGraphSCCAnalysis() :
		ModulePass(ID), g(NULL) {
	}

	void getAnalysisUsage(AnalysisUsage &AU) const {

		AU.addRequired<ModuleLoopControllersDepGraph> ();
		AU.setPreservesAll();
	}

	bool runOnModule(Module& M) {

		ModuleLoopControllersDepGraph& DepGraphPass = getAnalysis<ModuleLoopControllersDepGraph> ();
		DepGraph *g = DepGraphPass.depGraph;

		//g->unifyBackEdges();

		std::map<int, std::set<GraphNode*> > SCCs = g->getSCCs();

		NumSCCs += SCCs.size();

		MaxNumFunctionsInSCC = 0;
		MaxSCCSize = 0;
		NumInterproceduralSCCs = 0;
		NumSCCWithTwoOrMoreNodes = 0;

		for(std::map<int, std::set<GraphNode*> >::iterator it = SCCs.begin(); it!=SCCs.end(); it++){

			if (MaxSCCSize < it->second.size()) MaxSCCSize = it->second.size();

			if (it->second.size() > 1) NumSCCWithTwoOrMoreNodes++;

			//Collect list of functions of the SCC
			std::set<Function*> SCCFunctions;
			for(std::set<GraphNode*>::iterator node = it->second.begin(); node != it->second.end(); node++) {
				GraphNode* N = *node;
				if (OpNode* On = dyn_cast<OpNode>(N)){
					if(Value* V = On->getOperation()){
						if(Instruction* I = dyn_cast<Instruction>(V)) {
							Function* F = I->getParent()->getParent();
							if(!SCCFunctions.count(F)) SCCFunctions.insert(F);
						}
					}
				}
				if (VarNode* Vn = dyn_cast<VarNode>(N)){
					if(Value* V = Vn->getValue()){
						if(Instruction* I = dyn_cast<Instruction>(V)) {
							Function* F = I->getParent()->getParent();
							if(!SCCFunctions.count(F)) SCCFunctions.insert(F);
						}
					}
				}
			}

			if (MaxNumFunctionsInSCC < SCCFunctions.size()) MaxNumFunctionsInSCC = SCCFunctions.size();

			if (SCCFunctions.size() > 1) NumInterproceduralSCCs++;

		}

		return false;
	}
};

char ModuleDepGraphSCCAnalysis::ID = 0;
static RegisterPass<ModuleDepGraphSCCAnalysis> Y("module-depgraph-scc-analysis",
		"SCC Analysis of Dependence Graph (Module)");

} /* namespace llvm */


