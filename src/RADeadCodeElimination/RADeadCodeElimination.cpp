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
			if (range1.u.slt(range2.l)) {
				replaceAllUses(I, constTrue);
				hasSolved = true;
			}

			//Always false
			if (range1.l.sge(range2.u)) {
				replaceAllUses(I, constFalse);
				hasSolved = true;
			}

			break;


		case CmpInst::ICMP_ULT:

			//Always true
			if (range1.u.ult(range2.l)) {
				replaceAllUses(I, constTrue);
				hasSolved = true;
			}

			//Always false
			if (range1.l.uge(range2.u)) {
				replaceAllUses(I, constFalse);
				hasSolved = true;
			}

			break;


		case CmpInst::ICMP_SLE:

			//Always true
			if (range1.u.sle(range2.l)) {
				replaceAllUses(I, constTrue);
				hasSolved = true;
			}

			//Always false
			if (range1.l.sgt(range2.u)) {
				replaceAllUses(I, constFalse);
				hasSolved = true;
			}

			break;

		case CmpInst::ICMP_ULE:

			//Always true
			if (range1.u.ule(range2.l)) {
				replaceAllUses(I, constTrue);
				hasSolved = true;
			}

			//Always false
			if (range1.l.ugt(range2.u)) {
				replaceAllUses(I, constFalse);
				hasSolved = true;
			}

			break;


		case CmpInst::ICMP_SGT:

			//Always true
			if (range1.l.sgt(range2.u)) {
				replaceAllUses(I, constTrue);
				hasSolved = true;
			}

			//Always false
			if (range1.u.sle(range2.l)) {
				replaceAllUses(I, constFalse);
				hasSolved = true;
			}

			break;

		case CmpInst::ICMP_UGT:

			//Always true
			if (range1.l.ugt(range2.u)) {
				replaceAllUses(I, constTrue);
				hasSolved = true;
			}

			//Always false
			if (range1.u.ule(range2.l)) {
				replaceAllUses(I, constFalse);
				hasSolved = true;
			}

			break;

		case CmpInst::ICMP_SGE:

			//Always true
			if (range1.l.sge(range2.u)) {
				replaceAllUses(I, constTrue);
				hasSolved = true;
			}

			//Always false
			if (range1.u.slt(range2.l)) {
				replaceAllUses(I, constFalse);
				hasSolved = true;
			}

			break;

		case CmpInst::ICMP_UGE:

			//Always true
			if (range1.l.uge(range2.u)) {
				replaceAllUses(I, constTrue);
				hasSolved = true;
			}

			//Always false
			if (range1.u.ult(range2.l)) {
				replaceAllUses(I, constFalse);
				hasSolved = true;
			}

			break;

		case CmpInst::ICMP_EQ:

			//Always true : Two constants with the same value
			if (range1.l.eq(range2.l) && range1.u.eq(range2.u) && range1.l.eq(range1.u)) {
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
			if (range1.l.eq(range2.l) && range1.u.eq(range2.u) && range1.l.eq(range1.u)) {
				replaceAllUses(I, constFalse);
				hasSolved = true;
			}

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





	switch(I->getOpcode()){

	case Instruction::And:
	case Instruction::Or:
	case Instruction::Xor:

		return true;

	}

	if (hasSolved) deadInstructions.insert(I);

	return hasSolved;

}


bool ::RADeadCodeElimination::removeDeadBlocks() {

	return true;
}

void ::RADeadCodeElimination::replaceAllUses(Value* valueToReplace,
		Value* replaceWith) {
}

void ::RADeadCodeElimination::init(Module &M) {

	module = &M;
	context = module->getContext();

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
