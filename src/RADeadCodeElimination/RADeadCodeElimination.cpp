/*
 * RADeadCodeElimination.cpp
 *
 *  Created on: Mar 21, 2013
 *      Author: raphael
 */

#include "RADeadCodeElimination.h"

bool RADeadCodeElimination::runOnModule(Module &M) {

	init(M);

	bool hasChange = false;

	hasChange = hasChange || solveICmpInstructions();
	hasChange = hasChange || solveBooleanOperations();

	removeDeadInstructions();

	hasChange = hasChange || removeDeadCFGEdges();
	hasChange = hasChange || removeDeadBlocks();

	return hasChange;
}

bool ::RADeadCodeElimination::solveICmpInstructions() {

	bool hasChange = false;

	for (Module::iterator Fit = module->begin(), Fend = module->end(); Fit != Fend; Fit++){
		for (Function::iterator BBit = Fit->begin(), BBend = Fit->end(); BBit != BBend; BBit++){
			for (BasicBlock::iterator Iit = BBit->begin(), Iend = BBit->end(); Iit != Iend; Iit++){

				if (ICmpInst* CI = dyn_cast<ICmpInst>(Iit)){

					//We are only dealing with scalar values.
					if (CI->getOperand(0)->getType()->isIntegerTy())
						hasChange = hasChange || solveICmpInstruction(CI);

				}

			}
		}
	}

	return hasChange;
}

bool ::RADeadCodeElimination::solveICmpInstruction(ICmpInst* I) {

	Range range1 = ra->getRange(I->getOperand(0));
	Range range2 = ra->getRange(I->getOperand(1));
	bool hasSolved = false;

	if (isLimited(range1) && isLimited(range2)) {

		switch (I->getPredicate()){

		case CmpInst::ICMP_SLT:

			//Always true
			if (range1.getUpper().slt(range2.getLower())) {
				replaceAllUses(I, constTrue);
				hasSolved = true;
			}

			//Always false
			if (range1.getLower().sge(range2.getUpper())) {
				replaceAllUses(I, constFalse);
				hasSolved = true;
			}

			break;


		case CmpInst::ICMP_ULT:

			//Always true
			if (range1.getUpper().ult(range2.getLower())) {
				replaceAllUses(I, constTrue);
				hasSolved = true;
			}

			//Always false
			if (range1.getLower().uge(range2.getUpper())) {
				replaceAllUses(I, constFalse);
				hasSolved = true;
			}

			break;


		case CmpInst::ICMP_SLE:

			//Always true
			if (range1.getUpper().sle(range2.getLower())) {
				replaceAllUses(I, constTrue);
				hasSolved = true;
			}

			//Always false
			if (range1.getLower().sgt(range2.getUpper())) {
				replaceAllUses(I, constFalse);
				hasSolved = true;
			}

			break;

		case CmpInst::ICMP_ULE:

			//Always true
			if (range1.getUpper().ule(range2.getLower())) {
				replaceAllUses(I, constTrue);
				hasSolved = true;
			}

			//Always false
			if (range1.getLower().ugt(range2.getUpper())) {
				replaceAllUses(I, constFalse);
				hasSolved = true;
			}

			break;


		case CmpInst::ICMP_SGT:

			//Always true
			if (range1.getLower().sgt(range2.getUpper())) {
				replaceAllUses(I, constTrue);
				hasSolved = true;
			}

			//Always false
			if (range1.getUpper().sle(range2.getLower())) {
				replaceAllUses(I, constFalse);
				hasSolved = true;
			}

			break;

		case CmpInst::ICMP_UGT:

			//Always true
			if (range1.getLower().ugt(range2.getUpper())) {
				replaceAllUses(I, constTrue);
				hasSolved = true;
			}

			//Always false
			if (range1.getUpper().ule(range2.getLower())) {
				replaceAllUses(I, constFalse);
				hasSolved = true;
			}

			break;

		case CmpInst::ICMP_SGE:

			//Always true
			if (range1.getLower().sge(range2.getUpper())) {
				replaceAllUses(I, constTrue);
				hasSolved = true;
			}

			//Always false
			if (range1.getUpper().slt(range2.getLower())) {
				replaceAllUses(I, constFalse);
				hasSolved = true;
			}

			break;

		case CmpInst::ICMP_UGE:

			//Always true
			if (range1.getLower().uge(range2.getUpper())) {
				replaceAllUses(I, constTrue);
				hasSolved = true;
			}

			//Always false
			if (range1.getUpper().ult(range2.getLower())) {
				replaceAllUses(I, constFalse);
				hasSolved = true;
			}

			break;

		case CmpInst::ICMP_EQ:

			//Always true : Two constants with the same value
			if (range1.getLower().eq(range2.getLower()) && range1.getUpper().eq(range2.getUpper()) && range1.getLower().eq(range1.getUpper())) {
				replaceAllUses(I, constTrue);
				hasSolved = true;
			}

			//Always false
			if (range1.intersectWith(range2).isEmpty()) {
				replaceAllUses(I, constFalse);
				hasSolved = true;
			}

			break;

		case CmpInst::ICMP_NE:

			//Always true
			if (range1.intersectWith(range2).isEmpty()) {
				replaceAllUses(I, constTrue);
				hasSolved = true;
			}

			//Always false : Two constants with the same value
			if (range1.getLower().eq(range2.getLower()) && range1.getUpper().eq(range2.getUpper()) && range1.getLower().eq(range1.getUpper())) {
				replaceAllUses(I, constFalse);
				hasSolved = true;
			}

			break;

		default:
			break;

		}



	}

	if (hasSolved) deadInstructions.insert(I);

	return hasSolved;

}


bool ::RADeadCodeElimination::isValidBooleanOperation(Instruction* I) {

	switch(I->getOpcode()){

	case Instruction::And:
	case Instruction::Or:
	case Instruction::Xor:

		return true;

	}

	return false;

}

bool ::RADeadCodeElimination::solveBooleanOperations() {

	bool hasChange = false;

	for (Module::iterator Fit = module->begin(), Fend = module->end(); Fit != Fend; Fit++){
		for (Function::iterator BBit = Fit->begin(), BBend = Fit->end(); BBit != BBend; BBit++){
			for (BasicBlock::iterator Iit = BBit->begin(), Iend = BBit->end(); Iit != Iend; Iit++){

				if (isValidBooleanOperation(dyn_cast<Instruction>(Iit))){

					hasChange = hasChange || solveBooleanOperation(dyn_cast<Instruction>(Iit));

				}

			}
		}
	}

	return hasChange;
}

bool ::RADeadCodeElimination::solveBooleanOperation(Instruction* I) {

	bool hasSolved = false;

	//Just a protection...
	if (I->getNumOperands() != 2) return hasSolved;

	//Only handle operations between integers with 1 bit
	if (isConstantValue(I->getOperand(0))
			&& isConstantValue(I->getOperand(1))
			&& I->getOperand(1)->getType()->isIntegerTy(1)) {


		bool op1, op2;

		if ( ConstantInt* CI = dyn_cast<ConstantInt>(I->getOperand(0))  ) {
			op1 = CI->getValue().eq(APInt(1, 1));
		} else {
			op1 = ra->getRange(I->getOperand(0)).getUpper().eq(APInt(1, 1));
		}

		if ( ConstantInt* CI = dyn_cast<ConstantInt>(I->getOperand(1))  ) {
			op2 = CI->getValue().eq(APInt(1, 1));
		} else {
			op2 = ra->getRange(I->getOperand(1)).getUpper().eq(APInt(1, 1));
		}

		switch(I->getOpcode()){

		case Instruction::And:

			if (op1 && op2) replaceAllUses(I, constTrue);
			else replaceAllUses(I, constFalse);

		case Instruction::Or:
			if (op1 || op2) replaceAllUses(I, constTrue);
			else replaceAllUses(I, constFalse);

		case Instruction::Xor:

			if (op1 xor op2) replaceAllUses(I, constTrue);
			else replaceAllUses(I, constFalse);

		}

		hasSolved = true;

		deadInstructions.insert(I);

	}

	return hasSolved;

}


bool ::RADeadCodeElimination::removeDeadCFGEdges() {

	/*
	 * Logic behind this function:
	 * 	- We remove some edges from our CFG, which we can prove that will never be taken.
	 * 	We do this by replacing branch instructions with unconditional branches
	 */
	bool hasChange = false;

	for (Module::iterator Fit = module->begin(), Fend = module->end(); Fit != Fend; Fit++){
		for (Function::iterator BBit = Fit->begin(), BBend = Fit->end(); BBit != BBend; BBit++){

			//For each basic block, simplify the CFG if the terminator instruction relies on a constant
			hasChange = hasChange || ConstantFoldTerminator(BBit);

		}
	}

	return hasChange;

}


bool ::RADeadCodeElimination::removeDeadBlocks() {

	/*
	 * Apply algorithm to find the fixed point to remove the dead blocks:
	 *
	 * - Never remove the entry block
	 *
	 * - Remove blocks that don't have edges coming to it from blocks not dominated by it
	 * - Remove blocks that are reached only by dead blocks
	 */
	bool hasChange = false;
	unsigned int numDeadBlocks = deadBlocks.size();

	for (Module::iterator Fit = module->begin(), Fend = module->end(); Fit != Fend; Fit++){

		DominatorTree &DT = getAnalysis<DominatorTree>(*Fit);

		do {
			numDeadBlocks = deadBlocks.size();

			for (Function::iterator BBit = Fit->begin(), BBend = Fit->end(); BBit != BBend; BBit++){

				//We never remove the entry block
				if (&(Fit->getEntryBlock()) == BBit) continue;


				bool dead = true;

				for (pred_iterator PI = pred_begin(BBit), E = pred_end(BBit); PI != E; ++PI) {

					BasicBlock *PredBB = *PI;

					//If we find some predecessor not dominated by the block that still alive, the block still alive as well
					if ( !DT.dominates(BBit, PredBB) && deadBlocks.count(PredBB) == 0 ) {
						dead = false;
						break;
					}

				}

				if (dead) deadBlocks.insert(BBit);

			}

		} while (numDeadBlocks != deadBlocks.size());

	}


	if (deadBlocks.size() > 0) {

		hasChange = true;

		for( std::set<BasicBlock*>::iterator i = deadBlocks.begin(), end = deadBlocks.end(); i != end; i++){
			(*i)->eraseFromParent();
		}

		deadBlocks.clear();
	}

	return hasChange;

}

bool ::RADeadCodeElimination::ConstantFoldTerminator(BasicBlock *BB) {
	TerminatorInst *T = BB->getTerminator();

	// Branch - See if we are conditional jumping on constant
	if (BranchInst *BI = dyn_cast<BranchInst>(T)) {
		if (BI->isUnconditional()) return false;  // Can't optimize uncond branch
		BasicBlock *Dest1 = BI->getSuccessor(0);
		BasicBlock *Dest2 = BI->getSuccessor(1);

		if (ConstantInt *Cond = dyn_cast<ConstantInt>(BI->getCondition())) {
			// Are we branching on constant?
			// YES.  Change to unconditional branch...
			BasicBlock *Destination = Cond->getZExtValue() ? Dest1 : Dest2;
			BasicBlock *OldDest     = Cond->getZExtValue() ? Dest2 : Dest1;

			//cerr << "Function: " << T->getParent()->getParent()
			//     << "\nRemoving branch from " << T->getParent()
			//     << "\n\nTo: " << OldDest << endl;

			// Let the basic block know that we are letting go of it.  Based on this,
			// it will adjust it's PHI nodes.
			assert(BI->getParent() && "Terminator not inserted in block!");
			OldDest->removePredecessor(BI->getParent());

			// Set the unconditional destination, and change the insn to be an
			// unconditional branch.
			setUnconditionalDest(BI, Destination);
			return true;
		} else if (Dest2 == Dest1) {       // Conditional branch to same location?
			// This branch matches something like this:
			//     br bool %cond, label %Dest, label %Dest
			// and changes it into:  br label %Dest

			// Let the basic block know that we are letting go of one copy of it.
			assert(BI->getParent() && "Terminator not inserted in block!");
			Dest1->removePredecessor(BI->getParent());

			// Change a conditional branch to unconditional.
			setUnconditionalDest(BI, Dest1);
			return true;
		}
	}

	//TODO: Handle Switch Instructions

//	else if (SwitchInst *SI = dyn_cast<SwitchInst>(T)) {
//		// If we are switching on a constant, we can convert the switch into a
//		// single branch instruction!
//		if (ConstantInt *CI = dyn_cast<ConstantInt>(SI->getCondition())) {
//			BasicBlock *TheOnlyDest = SI->getSuccessor(0);  // The default dest
//			BasicBlock *DefaultDest = TheOnlyDest;
//			assert(TheOnlyDest == SI->getDefaultDest() &&
//				   "Default destination is not successor #0?");
//
//			// Figure out which case it goes to...
//			for (unsigned i = 1, e = SI->getNumSuccessors(); i != e; ++i) {
//				// Found case matching a constant operand?
//				if (SI->getSuccessorValue(i) == CI) {
//					TheOnlyDest = SI->getSuccessor(i);
//					break;
//				}
//
//				// Check to see if this branch is going to the same place as the default
//				// dest.  If so, eliminate it as an explicit compare.
//				if (SI->getSuccessor(i) == DefaultDest) {
//					// Remove this entry...
//					DefaultDest->removePredecessor(SI->getParent());
//					SI->removeCase(i);
//					--i; --e;  // Don't skip an entry...
//					continue;
//				}
//
//				// Otherwise, check to see if the switch only branches to one destination.
//				// We do this by reseting "TheOnlyDest" to null when we find two non-equal
//				// destinations.
//				if (SI->getSuccessor(i) != TheOnlyDest) TheOnlyDest = 0;
//			}
//
//			if (CI && !TheOnlyDest) {
//				// Branching on a constant, but not any of the cases, go to the default
//				// successor.
//				TheOnlyDest = SI->getDefaultDest();
//			}
//
//			// If we found a single destination that we can fold the switch into, do so
//			// now.
//			if (TheOnlyDest) {
//				// Insert the new branch..
//				BranchInst::Create(TheOnlyDest, SI);
//				BasicBlock *BB = SI->getParent();
//
//				// Remove entries from PHI nodes which we no longer branch to...
//				for (unsigned i = 0, e = SI->getNumSuccessors(); i != e; ++i) {
//				// Found case matching a constant operand?
//				BasicBlock *Succ = SI->getSuccessor(i);
//				if (Succ == TheOnlyDest)
//					TheOnlyDest = 0;  // Don't modify the first branch to TheOnlyDest
//				else
//					Succ->removePredecessor(BB);
//				}
//
//				// Delete the old switch...
//				SI->eraseFromParent();
//				return true;
//			} else if (SI->getNumSuccessors() == 2) {
//				// Otherwise, we can fold this switch into a conditional branch
//				// instruction if it has only one non-default destination.
//				Value *Cond = new ICmpInst(ICmpInst::ICMP_EQ, SI->getCondition(),
//										 SI->getSuccessorValue(1), "cond", SI);
//				// Insert the new branch...
//				BranchInst::Create(SI->getSuccessor(1), SI->getSuccessor(0), Cond, SI);
//
//				// Delete the old switch...
//				SI->eraseFromParent();
//				return true;
//			}
//		} else {
//
//
//
//
//
//
//		}
//	}
	return false;
}


void ::RADeadCodeElimination::setUnconditionalDest(BranchInst *BI, BasicBlock *Destination){

	BranchInst::Create(Destination, BI);
	BI->eraseFromParent();

}

void ::RADeadCodeElimination::replaceAllUses(Value* valueToReplace,
		Value* replaceWith) {

	for (Value::use_iterator uit = valueToReplace->use_begin(), uend = valueToReplace->use_end(); uit != uend; ++uit) {

		if (Instruction* I = dyn_cast<Instruction>(*uit)){

			if (I == replaceWith) {
				continue;
			}

			I->replaceUsesOfWith(valueToReplace, replaceWith);
		}
	}
}

void ::RADeadCodeElimination::init(Module &M) {

	module = &M;
	context = &(module->getContext());

	constFalse = ConstantInt::get(Type::getInt1Ty(*context), 0);
	constTrue = ConstantInt::get(Type::getInt1Ty(*context), 1);

	ra = &getAnalysis<InterProceduralRA<Cousot> >();
	Min = ra->getMin();
	Max = ra->getMax();
}

void ::RADeadCodeElimination::removeDeadInstructions() {

	for( std::set<Instruction*>::iterator i = deadInstructions.begin(), end = deadInstructions.end(); i != end; i++){
		(*i)->eraseFromParent();
	}

	deadInstructions.clear();
}

bool ::RADeadCodeElimination::isConstantValue(Value* V) {

	if (isa<Constant>(V)) return true;

	Range r = ra->getRange(V);
	return (isLimited(r) && r.getUpper().eq(r.getLower()));

}
