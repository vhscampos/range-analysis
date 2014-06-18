/*
 * LoopNormalizer.cpp
 *
 *  Created on: Dec 10, 2013
 *      Author: raphael
 */
#ifndef DEBUG_TYPE
#define DEBUG_TYPE "LoopNormalizer"
#endif

#include "LoopNormalizer.h"

STATISTIC(NumNormalizedLoops, "Number of Normalized Loops");

using namespace llvm;

void LoopNormalizer::switchBranch(BasicBlock* FromBlock, BasicBlock* PreviousTo, BasicBlock* NewTo){

	TerminatorInst* T = FromBlock->getTerminator();

	if(BranchInst* BI = dyn_cast<BranchInst>(T)){
		if (BI->isUnconditional()){
			//Easiest case; We just have to replace the instruction
			BranchInst::Create(NewTo, BI);
			BI->eraseFromParent();
		} else{
			//We iterate over the successors to find which one leads to our previous successor
			for (unsigned int i = 0; i < BI->getNumSuccessors(); i++){
				if (BI->getSuccessor(i) == PreviousTo){
					BI->setSuccessor(i, NewTo);
				}
			}
		}

	} else if (SwitchInst* SI = dyn_cast<SwitchInst>(T)) {

		//We iterate over the successors to find which one leads to our previous successor
		for (unsigned int i = 0; i < SI->getNumSuccessors(); i++){
			if (SI->getSuccessor(i) == PreviousTo){
				SI->setSuccessor(i, NewTo);
			}
		}

	}else {
		//There is something wrong. Abort.
		errs() << *FromBlock << "\n\n" << *PreviousTo << "\n";


		assert(false && "TerminatorInst neither is a BranchInst nor is a SwitchInst.");
	}

}

BasicBlock* LoopNormalizer::normalizePreHeaders(std::set<BasicBlock*> PreHeaders, BasicBlock* Header){

	BasicBlock* EntryBlock = BasicBlock::Create(Header->getParent()->getContext(), "Entry", Header->getParent(), Header);

	//Unconditional branch from the entry block to the loop header
	BranchInst* br = BranchInst::Create(Header,EntryBlock);

	//Disconnect the PreHeaders from the Header and Connect them to the Entry Block
	for(std::set<BasicBlock*>::iterator it = PreHeaders.begin(), iend = PreHeaders.end(); it != iend; it++){
		switchBranch(*it, Header, EntryBlock);
	}


	//Now the PHIs are a mess. Here we set them accordingly
	for(BasicBlock::iterator it = Header->begin(); it != Header->end(); it++){
		if(PHINode* PHI = dyn_cast<PHINode>(it)){

			std::set<unsigned int> OutsideIncomingValues;
			for (unsigned int i = 0; i < PHI->getNumIncomingValues(); i++){
				if (PreHeaders.count(PHI->getIncomingBlock(i))) OutsideIncomingValues.insert(i);
			}

			unsigned int numIncomingValues = OutsideIncomingValues.size();

			//Only one value: just change the incoming block
			if (numIncomingValues == 1) {
				PHI->setIncomingBlock(*OutsideIncomingValues.begin(), EntryBlock);
			}

			//More than one value: we must create a PHI in the EntryBlock and use its value in the Header
			else if (numIncomingValues > 1) {
				PHINode* newPHI = PHINode::Create(PHI->getType(), numIncomingValues, "", EntryBlock->getTerminator());

				for (std::set<unsigned int>::iterator i = OutsideIncomingValues.begin(); i != OutsideIncomingValues.end(); i++){
					newPHI->addIncoming( PHI->getIncomingValue(*i), PHI->getIncomingBlock(*i) );
				}

				PHI->addIncoming(newPHI, EntryBlock);

				for (unsigned int i = 0; i < newPHI->getNumIncomingValues(); i++){
					PHI->removeIncomingValue(newPHI->getIncomingBlock(i), false );
				}

			}

		}
		else break;
	}


	return EntryBlock;
}

void LoopNormalizer::normalizePostExit(BasicBlock* exitBlock, BasicBlock* postExitBlock){

	if (!postExitBlock->isLandingPad()) {

		//LandingPads are already normalized


		BasicBlock* newBB = BasicBlock::Create(exitBlock->getParent()->getParent()->getContext(),
												"", exitBlock->getParent(), postExitBlock);
		BranchInst::Create(postExitBlock, newBB);

		switchBranch(exitBlock,postExitBlock,newBB);

		//Replace the incoming blocks of the PHIs
		for(BasicBlock::iterator i = postExitBlock->begin(); i != postExitBlock->end(); i++){

			Instruction* I = i;
			if(PHINode* PHI = dyn_cast<PHINode>(I)){
				int idx = PHI->getBasicBlockIndex(exitBlock);
				if(idx >= 0) {
					PHI->setIncomingBlock(idx, newBB);
				}
			} else {
				break;
			}

		}
	}

}


bool llvm::LoopNormalizer::doInitialization(Module& M) {
	NumNormalizedLoops = 0;
	return false;
}

/*
 * This pass groups all the incoming blocks that a loop header have into
 * one single basic block. We call this basic block  the "Entry Block" of
 * a loop.
 * The entry block is a basic block that is executed only once before the first
 * iteration of the loop.
 *
 * Moreover, we normalize the exits of the loop such that when the loop stop,
 * the control goes to a block that does not belong to the loop but is
 * dominated by the entry block. We call blocks like that as "Post Exit Blocks".
 * A loop may have more than one post exit block, if it has more than one exit point
 * (e.g. when the loop has a break instruction).
 */
bool llvm::LoopNormalizer::runOnFunction(Function& F) {

	LoopInfoEx& li = getAnalysis<LoopInfoEx>();

	//Normalize headers
	for (LoopInfoEx::iterator it = li.begin(); it!= li.end(); it++){

		Loop* loop = *it;

		BasicBlock* header = loop->getHeader();

		std::set<BasicBlock*> OutsidePreHeaders;

		for(pred_iterator pred = pred_begin(header); pred != pred_end(header); pred++){

			BasicBlock* predecessor = *pred;

			if (li.getLoopFor(predecessor) != li.getLoopFor(header)){
				OutsidePreHeaders.insert(predecessor);
			}

		}

		normalizePreHeaders(OutsidePreHeaders, header);

		NumNormalizedLoops++;

	}


//	//Normalize exits
//	std::set<BasicBlock*> loopExits = nla.getLoopExitBlocks();
//	for (std::set<BasicBlock*>::iterator it = loopExits.begin(); it!= loopExits.end(); it++){
//
//		BasicBlock* exitBlock = *it;
//
//		for(succ_iterator succ = succ_begin(exitBlock); succ != succ_end(exitBlock); succ++){
//
//			BasicBlock* successor = *succ;
//
//			if (li.getLoopDepth(successor) < li.getLoopDepth(exitBlock)){
//
//				//Successor is outside the block
//				normalizePostExit(exitBlock, successor);
//
//			}
//
//		}
//
//	}

	return true;
}


char LoopNormalizer::ID = 0;
static RegisterPass<LoopNormalizer> Y("loop-normalizer",
                "Loop Normalizer");
