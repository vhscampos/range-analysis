/*
 * LoopStructure.cpp
 *
 *  Created on: Jan 10, 2014
 *      Author: raphael
 */

#ifndef DEBUG_TYPE
#define DEBUG_TYPE "LoopStructure"
#endif

#include "LoopStructure.h"

using namespace llvm;

STATISTIC(NumLoops			, "Total Number of Loops");
STATISTIC(NumNestedLoops	, "Number of CFG Nested Loops");
STATISTIC(NumLoopsSingleExit, "Number of Loops with a Single Exit point");
STATISTIC(NumUnhandledExits	, "Number of not handled exit points (not branch or switch)");

STATISTIC(NumIntervalLoops	, "Number of Interval Loops");
STATISTIC(NumEqualityLoops	, "Number of Equality Loops");
STATISTIC(NumOtherLoops		, "Number of Other Loops");


STATISTIC(NumSingleNodeSCCs	, "Number of Single-Node DepGraph SCCs");
STATISTIC(NumMultiNodeSCCs	, "Number of Multi-Node  DepGraph SCCs");

STATISTIC(NumSinglePathSCCs	, "Number of Single-Path DepGraph SCCs");
STATISTIC(NumMultiPathSCCs	, "Number of Multi-Path  DepGraph SCCs");

STATISTIC(NumSingleLoopSCCs	, "Number of Single-Loop DepGraph SCCs");
STATISTIC(NumNestedLoopSCCs	, "Number of Nested-Loop DepGraph SCCs");

/*
 * Function getDelta
 *
 * Given two sets, generate the list of items that are present in the first set
 * but are not present on the second.
 */
template <typename T>
std::set<T> getDelta(std::set<T> t1, std::set<T> t2) {

	std::set<T> result;
	for(typename std::set<T>::iterator it = t1.begin(), iend = t1.end(); it != iend;  it++ ) {
		T item = *it;
		if (!t2.count(item)) {
			result.insert(item);
		}
	}

	return result;

}


bool LoopStructure::doInitialization(Module& M) {

	NumLoops = 0;
	NumNestedLoops = 0;
	NumLoopsSingleExit = 0;
	NumUnhandledExits = 0;

	NumSingleNodeSCCs = 0;
	NumMultiNodeSCCs = 0;
	NumSinglePathSCCs = 0;
	NumMultiPathSCCs = 0;
	NumSingleLoopSCCs = 0;
	NumNestedLoopSCCs = 0;

	NumIntervalLoops = 0;
	NumEqualityLoops = 0;
	NumOtherLoops = 0;



	return true;
}

bool LoopStructure::isIntervalComparison(ICmpInst* CI){

	switch(CI->getPredicate()){
	case ICmpInst::ICMP_SGE:
	case ICmpInst::ICMP_SGT:
	case ICmpInst::ICMP_UGE:
	case ICmpInst::ICMP_UGT:
	case ICmpInst::ICMP_SLE:
	case ICmpInst::ICMP_SLT:
	case ICmpInst::ICMP_ULE:
	case ICmpInst::ICMP_ULT:
		return true;
		break;
	default:
		return false;
	}

}

BasicBlock* findLoopControllerBlock(Loop* l){

	BasicBlock* header = l->getHeader();

	SmallVector<BasicBlock*, 2>  exitBlocks;
	l->getExitingBlocks(exitBlocks);

	//Case 1: for/while (the header is an exiting block)
	for (SmallVectorImpl<BasicBlock*>::iterator It = exitBlocks.begin(), Iend = exitBlocks.end(); It != Iend; It++){
		BasicBlock* BB = *It;
		if (BB == header) {
			return BB;
		}
	}

	//Case 2: repeat-until/do-while (the exiting block can branch directly to the header)
	for (SmallVectorImpl<BasicBlock*>::iterator It = exitBlocks.begin(), Iend = exitBlocks.end(); It != Iend; It++){

		BasicBlock* BB = *It;

		//Here we iterate over the successors of BB to check if the header is one of them
		for(succ_iterator s = succ_begin(BB); s != succ_end(BB); s++){

			if (*s == header) {
				return BB;
			}
		}

	}

	//Otherwise, return the first exit block
	return *(exitBlocks.begin());
}

bool LoopStructure::runOnFunction(Function& F) {

//	std::string Filename = "/tmp/DepGraphLoopPaths.txt";
//	std::string ErrorInfo;
//	raw_fd_ostream File(Filename.c_str(), ErrorInfo, raw_fd_ostream::F_Append);
//	if (!ErrorInfo.empty()){
//	  errs() << "Error opening file /tmp/DepGraphLoopPaths.txt for writing! Error Info: " << ErrorInfo  << " \n";
//	  return false;
//	}

	std::set<int> analyzedSCCs;
	std::set<GraphNode*> visitedNodes;

	LoopNormalizerAnalysis& ln = getAnalysis<LoopNormalizerAnalysis>();

	LoopControllersDepGraph & lcd = getAnalysis<LoopControllersDepGraph>();
	DepGraph* graph = lcd.depGraph;
	graph->recomputeSCCs();

	LoopInfoEx & li = getAnalysis<LoopInfoEx>();
	for (LoopInfoEx::iterator it = li.begin(); it != li.end();  it++) {

		// Section 1: Structure of the loops in the CFG
		NumLoops++;

		Loop* L = *it;

		if (L->getLoopDepth() > 1) NumNestedLoops++;

		SmallVector<BasicBlock*, 4> exitingBlocks;
		L->getExitingBlocks(exitingBlocks);

		if (exitingBlocks.size() == 1) {
			NumLoopsSingleExit++;
		}


		//Section 2: Type of loops acording to its stop condition
		Loop* loop = L;

		BasicBlock* header = loop->getHeader();
		BasicBlock* entryBlock = ln.entryBlocks[header];

		/*
		 * Here we are looking for the predicate that stops the loop.
		 *
		 * At this moment, we are only considering loops that are controlled by
		 * integer comparisons.
		 */
		BasicBlock* exitBlock = findLoopControllerBlock(loop);
		assert(exitBlock && "ExitBlock not found!");

		TerminatorInst* T = exitBlock->getTerminator();
		BranchInst* BI = dyn_cast<BranchInst>(T);
		ICmpInst* CI = BI ? dyn_cast<ICmpInst>(BI->getCondition()) : NULL;

		int LoopClass = 0;

		if (!CI) {
			NumOtherLoops++;
		}
		else {
			if (isIntervalComparison(CI)) {
				NumIntervalLoops++;
			} else {
				NumEqualityLoops++;
			}
		}




		//Section 3: Structure of the loops in the Dep.Graph
		for (SmallVectorImpl<BasicBlock*>::iterator bIt = exitingBlocks.begin(); bIt != exitingBlocks.end(); bIt++){
			BasicBlock* exitingBlock = *bIt;

			TerminatorInst *T = exitingBlock->getTerminator();

			Value* Condition = NULL;

			if (BranchInst* BI = dyn_cast<BranchInst>(T)){
				Condition = BI->getCondition();
			} else if (SwitchInst* SI = dyn_cast<SwitchInst>(T)){
				Condition = SI->getCondition();
			} else {
				NumUnhandledExits++;
			}

			if (Condition) {

				if (GraphNode* ConditionNode = graph->findNode(Condition)) {

					std::set<GraphNode*> tmp = visitedNodes;
					std::map<int, GraphNode*> firstNodeVisitedPerSCC;

					//Avoid visiting the same node twice
					graph->dfsVisitBack_ext(ConditionNode, visitedNodes, firstNodeVisitedPerSCC);

					std::set<GraphNode*> delta = getDelta(visitedNodes, tmp);

					//Iterate over the dependencies
					for (std::set<GraphNode*>::iterator nIt = delta.begin(); nIt != delta.end(); nIt++ ) {

						GraphNode* CurrentNode = *nIt;

						int SCCID = graph->getSCCID(CurrentNode);

						if (graph->getSCC(SCCID).size() > 1) {

							//Count the number of paths only once per SCC
							if (!analyzedSCCs.count(SCCID)) {
								analyzedSCCs.insert(SCCID);

								NumMultiNodeSCCs++;

								GraphNode* firstNodeVisitedInSCC = firstNodeVisitedPerSCC[SCCID];

								std::set<std::stack<GraphNode*> > paths = graph->getAcyclicPathsInsideSCC(firstNodeVisitedInSCC, firstNodeVisitedInSCC);

								if (paths.size() == 1){
									NumSinglePathSCCs++;
								} else {
									NumMultiPathSCCs++;

									if (graph->hasNestedLoop(firstNodeVisitedInSCC)){
										NumNestedLoopSCCs++;
									} else {
										NumSingleLoopSCCs++;
									}

								}

							}

						} else {
							//Count the number of paths only once per SCC
							if (!analyzedSCCs.count(SCCID)) {
								analyzedSCCs.insert(SCCID);

								NumSingleNodeSCCs++;
							}
						}

					}
				}

			}

		}

	}


	//We don't make changes to the source code; return False
	return false;
}


char LoopStructure::ID = 0;
static RegisterPass<LoopStructure> Y("LoopStructure",
                "Loop Structure Analysis");
