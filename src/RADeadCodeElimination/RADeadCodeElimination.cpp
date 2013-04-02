/*
 * RADeadCodeElimination.cpp
 *
 *  Created on: Mar 21, 2013
 *      Author: raphael
 */
#define DEBUG_TYPE "RADeadCodeElimination"

#include "RADeadCodeElimination.h"

STATISTIC(InstructionsEliminated, "Number of instructions eliminated");
STATISTIC(BasicBlocksEliminated,  "Number of basic blocks entirely eliminated");
STATISTIC(ICmpInstructionsSolved, "Number of ICmp instructions solved");
STATISTIC(LogicalInstructionsSolved, "Number of binary logical instructions solved");

bool RADeadCodeElimination::runOnFunction(Function &F) {

	function = &F;

	context = &(function->getParent()->getContext());

	constFalse = ConstantInt::get(Type::getInt1Ty(*context), 0);
	constTrue = ConstantInt::get(Type::getInt1Ty(*context), 1);

	ra = &getAnalysis<InterProceduralRA<Cousot> >();
	Min = ra->getMin();
	Max = ra->getMax();

	bool hasChange = false;

	hasChange = solveICmpInstructions();

	while (solveBooleanOperations()) hasChange = true;

	removeDeadInstructions();

	hasChange = removeDeadCFGEdges() 		|| hasChange;
	hasChange = removeDeadBlocks() 			|| hasChange;
	hasChange = mergeBlocks() 				|| hasChange;

	return hasChange;
}

bool ::RADeadCodeElimination::solveICmpInstructions() {

	bool hasChange = false;

	for (Function::iterator BBit = function->begin(), BBend = function->end(); BBit != BBend; BBit++){
		for (BasicBlock::iterator Iit = BBit->begin(), Iend = BBit->end(); Iit != Iend; Iit++){

			if (ICmpInst* CI = dyn_cast<ICmpInst>(Iit)){

				//We are only dealing with scalar values.
				if (CI->getOperand(0)->getType()->isIntegerTy())
					hasChange = solveICmpInstruction(CI) || hasChange;

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

	if (hasSolved) {
		ICmpInstructionsSolved++;
		deadInstructions.insert(I);
	}

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

	for (Function::iterator BBit = function->begin(), BBend = function->end(); BBit != BBend; BBit++){
		for (BasicBlock::iterator Iit = BBit->begin(), Iend = BBit->end(); Iit != Iend; Iit++){

			if (isValidBooleanOperation(dyn_cast<Instruction>(Iit))){

				hasChange = solveBooleanOperation(dyn_cast<Instruction>(Iit)) || hasChange;

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
		LogicalInstructionsSolved++;
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

	for (Function::iterator BBit = function->begin(), BBend = function->end(); BBit != BBend; BBit++){

		//For each basic block, simplify the CFG if the terminator instruction relies on a constant
		hasChange = llvm::ConstantFoldTerminator(BBit, true, NULL) || hasChange;

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

	DominatorTree &DT = getAnalysis<DominatorTree>();

	do {
		numDeadBlocks = deadBlocks.size();

		for (Function::iterator BBit = function->begin(), BBend = function->end(); BBit != BBend; BBit++){

			//We never remove the entry block
			if (&(function->getEntryBlock()) == BBit) continue;


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


	if (deadBlocks.size() > 0) {

		hasChange = true;

		for( std::set<BasicBlock*>::iterator i = deadBlocks.begin(), end = deadBlocks.end(); i != end; i++){

			BasicBlocksEliminated++;

			(*i)->eraseFromParent();
		}

		deadBlocks.clear();
	}

	return hasChange;

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


void ::RADeadCodeElimination::removeDeadInstructions() {

	for( std::set<Instruction*>::iterator i = deadInstructions.begin(), end = deadInstructions.end(); i != end; i++){
		RecursivelyDeleteTriviallyDeadInstructions(*i, NULL);
		InstructionsEliminated++;
	}

	deadInstructions.clear();
}

bool ::RADeadCodeElimination::isConstantValue(Value* V) {

	if (isa<Constant>(V)) return true;

	Range r = ra->getRange(V);
	return (isLimited(r) && r.getUpper().eq(r.getLower()));

}

char RADeadCodeElimination::ID = 0;
static RegisterPass<RADeadCodeElimination>
X("ra-dce",
		"Dead Code Elimination based in Range Analysis");


BasicBlock* getSingleSuccessor(BasicBlock* BB){

	BasicBlock* result = NULL;


	for (succ_iterator SI = succ_begin(BB), E = succ_end(BB); SI != E; ++SI) {

		if (result)
			//More than one successor. Return NULL
			return NULL;

		//First successor
		result = *SI;

	}

	return result;

}

bool ::RADeadCodeElimination::mergeBlocks() {

	bool hasChange = false;

	for (Function::iterator BBit = function->begin(), BBend = function->end(); BBit != BBend; BBit++){

		//For each basic block, simplify the CFG if the basic block has only one predecessor and the predecessor has only one successor
		if (BasicBlock* Test = BBit->getSinglePredecessor()) {

			if (getSingleSuccessor(Test)) {
				llvm::MergeBasicBlockIntoOnlyPred(BBit, NULL);
				hasChange = true;
			}
		}
	}

	return hasChange;

}
