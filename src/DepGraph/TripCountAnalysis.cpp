/*
 * TripCountAnalysis.cpp
 *
 *  Created on: Dec 10, 2013
 *      Author: raphael
 */

#ifndef DEBUG_TYPE
#define DEBUG_TYPE "TripCountAnalysis"
#endif

#include "TripCountAnalysis.h"

using namespace llvm;


bool TripCountAnalysis::runOnFunction(Function& F){




	LoopNormalizerAnalysis& lna = getAnalysis<LoopNormalizerAnalysis>();
	LoopInfoEx& li = getAnalysis<LoopInfoEx>();

	for (LoopInfoEx::iterator it = li.begin(); it!= li.end(); it++){

		Loop* loop = *it;

		BasicBlock* entryBlock = lna.getEntryBlock(loop->getHeader());

		//If no trip count was found, it will be null.
		//
		//If there is no trip count for any loop, probably the pass
		//"Trip Count Generator" has not been executed.
		tripCounts[loop->getHeader()] = NULL;

		/*
		 * The Trip Count must be available in the entry block of the loop.
		 *
		 * Here we will look for it
		 */
		for(BasicBlock::iterator iit = entryBlock->begin(), iend = entryBlock->end(); iit != iend; iit++){
			Instruction* I = iit;

			if (IsTripCount(*I)) {
				tripCounts[loop->getHeader()] = I;
				break;
			}
		}

	}


	return false;
}

bool TripCountAnalysis::IsTripCount(Instruction& inst)
{
    return inst.getMetadata("TripCount") != 0;
}

char llvm::TripCountAnalysis::ID = 0;
static RegisterPass<TripCountAnalysis> X("tc-analysis","Trip Count Analysis");
