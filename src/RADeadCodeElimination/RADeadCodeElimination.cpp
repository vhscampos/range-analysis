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


bool ::RADeadCodeElimination::removeDeadBlocks() {

	return true;
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
