/*
 * SymbolicInterval.cpp
 *
 * Copyright (C) 2011-2012  Victor Hugo Sperle Campos
 * Copyright (C) 2013-2014  Raphael Ernani Rodrigues
 */

#include "SymbolicInterval.h"

using namespace llvm;

// ========================================================================== //
// SymbInterval
// ========================================================================== //

SymbInterval::SymbInterval(const Range& range, const Value* bound,
		CmpInst::Predicate pred) :
		BasicInterval(range), bound(bound), pred(pred) {
}

SymbInterval::~SymbInterval() {
}

void SymbInterval::fixIntersects(Range boundRange) {
	// Get the lower and the upper bound of the
	// node which bounds this intersection.
	APInt l = boundRange.getLower();
	APInt u = boundRange.getUpper();

	// Get the lower and upper bound of the interval of this operation
	APInt lower = this->getRange().getLower();
	APInt upper = this->getRange().getUpper();

	switch (this->getOperation()) {
	case ICmpInst::ICMP_EQ: // equal
		this->setRange(Range(l, u));
		break;
	case ICmpInst::ICMP_SLE: // signed less or equal
		this->setRange(Range(lower, u));
		break;
	case ICmpInst::ICMP_SLT: // signed less than
		if (u != Max) {
			this->setRange(Range(lower, u - 1));
		} else {
			this->setRange(Range(lower, u));
		}
		break;
	case ICmpInst::ICMP_SGE: // signed greater or equal
		this->setRange(Range(l, upper));
		break;
	case ICmpInst::ICMP_SGT: // signed greater than
		if (l != Min) {
			this->setRange(Range(l + 1, upper));
		} else {
			this->setRange(Range(l, upper));
		}
		break;
	default:
		this->setRange(Range(Min, Max));
	}
}

// Print name of variable according to its type
static void printVarName(const Value *V, raw_ostream& OS) {
	const Argument *A = NULL;
	const Instruction *I = NULL;

	if ((A = dyn_cast<Argument>(V))) {
		OS << A->getParent()->getName() << "." << A->getName();
	} else if ((I = dyn_cast<Instruction>(V))) {
		OS << I->getParent()->getParent()->getName() << "."
				<< I->getParent()->getName() << "." << I->getName();
	} else {
		OS << V->getName();
	}
}

/// Pretty print.
void SymbInterval::print(raw_ostream& OS) const {
	switch (this->getOperation()) {
	case ICmpInst::ICMP_EQ: // equal
		OS << "[lb(";
		printVarName(getBound(), OS);
		OS << "), ub(";
		printVarName(getBound(), OS);
		OS << ")]";
		break;
	case ICmpInst::ICMP_SLE: // sign less or equal
		OS << "[-inf, ub(";
		printVarName(getBound(), OS);
		OS << ")]";
		break;
	case ICmpInst::ICMP_SLT: // sign less than
		OS << "[-inf, ub(";
		printVarName(getBound(), OS);
		OS << ") - 1]";
		break;
	case ICmpInst::ICMP_SGE: // sign greater or equal
		OS << "[lb(";
		printVarName(getBound(), OS);
		OS << "), +inf]";
		break;
	case ICmpInst::ICMP_SGT: // sign greater than
		OS << "[lb(";
		printVarName(getBound(), OS);
		OS << " - 1), +inf]";
		break;
	default:
		OS << "Unknown Instruction.\n";
	}
}
