/*
 * LoopNormalizer.cpp
 *
 *  Created on: Dec 10, 2013
 *      Author: raphael
 */
#ifndef DEBUG_TYPE
#define DEBUG_TYPE "LoopNormalizerAnalysis"
#endif

#include "LoopNormalizerAnalysis.h"

using namespace llvm;

bool llvm::LoopNormalizerAnalysis::doInitialization(Module& M) {
	return false;
}

/*
 * This pass groups all the incoming blocks that a loop header have into
 * one single basic block. We call this basic block  the "Entry Block" of
 * a loop.
 *
 * The entry block is a basic block that is executed only once before the first
 * iteration of the loop.
 */
bool llvm::LoopNormalizerAnalysis::runOnFunction(Function& F) {

	LoopInfoEx& li = getAnalysis<LoopInfoEx>();

	for (LoopInfoEx::iterator it = li.begin(); it!= li.end(); it++){

		Loop* loop = *it;

		BasicBlock* header = loop->getHeader();

		int count = 0;

		for(pred_iterator pred = pred_begin(header); pred != pred_end(header); pred++){

			BasicBlock* predecessor = *pred;

			if (li.getLoopDepth(predecessor) < li.getLoopDepth(header)){
				entryBlocks[header] = predecessor;
				count++;

				assert(count < 2 && "Two entry blocks. Loop not normalized!");
			}

		}

	}

	//Returning false means that we do not modify the program at all
	return false;
}


char LoopNormalizerAnalysis::ID = 0;
static RegisterPass<LoopNormalizerAnalysis> Y("loop-normalizer-analysis",
		"Loop Normalizer Analysis", true, true);

BasicBlock* llvm::LoopNormalizerAnalysis::getEntryBlock(Loop* L) {
	return getEntryBlock(L->getHeader());
}

BasicBlock* llvm::LoopNormalizerAnalysis::getEntryBlock(
		BasicBlock* LoopHeader) {
	return entryBlocks[LoopHeader];
}
