//===-------------------------- RangeAnalysis.cpp -------------------------===//
//===----- Performs a range analysis of the variables of the function -----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#define DEBUG_TYPE "range-analysis"

#include "RangeAnalysis.h"
#include "llvm/Support/InstIterator.h"

using namespace llvm;

// These macros are used to get stats regarging the precision of our analysis.
STATISTIC(usedBits, "Initial number of bits.");
STATISTIC(needBits, "Needed bits.");
STATISTIC(totalInstructions, "Number of instructions of the function.");
STATISTIC(evaluatedInstructions, "Number of evaluated instructions.");
STATISTIC(percentReduction, "Percentage of reduction of the number of bits.");

// The number of bits needed to store the largest variable of the function (APInt).
unsigned MAX_BIT_INT = 1;

// This map is used to store the number of times that the widen_meet 
// operator is called on a variable. It was a Fernando's suggestion.
DenseMap<const Value*, unsigned> FerMap;

// ========================================================================== //
// Static global functions and definitions
// ========================================================================== //

// The min and max integer values for a given bit width.
APInt Min = APInt::getSignedMinValue(MAX_BIT_INT);
APInt Max = APInt::getSignedMaxValue(MAX_BIT_INT);

const std::string sigmaString = "vSSA_sigma";

// Used to print pseudo-edges in the Constraint Graph dot
std::string pestring;
raw_string_ostream pseudoEdgesString(pestring);

static void printVarName(const Value *V, raw_ostream& OS) {
	const Argument *A = NULL;
	const Instruction *I = NULL;
	
	if ((A = dyn_cast<Argument>(V))) {
		OS << A->getParent()->getName() << "." << A->getName() << " ";
	}
	else if ((I = dyn_cast<Instruction>(V))) {
		OS << I->getParent()->getParent()->getName() << "." << I->getParent()->getName() << "." << I->getName() << " ";
	}
	else {
		OS << V->getName() << " ";
	}
}

/// Selects the instructions that we are going to evaluate.
static bool isValidInstruction(const Instruction* I) {
	// FIXME: How can I reference a Phi function by its opcode?
	if (dyn_cast<PHINode>(I)) {
		return true;
	}

	switch (I->getOpcode()) {
	case Instruction::Add:
	case Instruction::Sub:
	case Instruction::Mul:
	case Instruction::UDiv:
	case Instruction::SDiv:
	case Instruction::URem:
	case Instruction::SRem:
	case Instruction::Shl:
	case Instruction::LShr:
	case Instruction::AShr:
	case Instruction::And:
	case Instruction::Or:
	case Instruction::Xor:
	case Instruction::Trunc:
	case Instruction::ZExt:
	case Instruction::SExt:
	case Instruction::Load:
	case Instruction::Store:
		return true;
	default: 
		return false;
	}

	assert(false && "It should not be here.");
	return false;
}


// ========================================================================== //
// Range Analysis
// ========================================================================== //

template <class CGT>
void RangeAnalysis<CGT>::getMaxBitWidth(const Function& F) {
	unsigned int InstBitSize = 0, opBitSize = 0;
	MAX_BIT_INT = 1;

	// Obtains the maximum bit width of the instructions of the function.
	for (const_inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
		if (!isValidInstruction(&*I)) {
			continue;
		}

		InstBitSize = I->getType()->getPrimitiveSizeInBits();
		if (I->getType()->isIntegerTy() && InstBitSize > MAX_BIT_INT) {
			MAX_BIT_INT = InstBitSize;
		}

		// Obtains the maximum bit width of the operands od the instruction.
		User::const_op_iterator bgn = I->op_begin(), end = I->op_end();
		for (; bgn != end; ++bgn) {
			opBitSize = (*bgn)->getType()->getPrimitiveSizeInBits();
			if ((*bgn)->getType()->isIntegerTy() && opBitSize > MAX_BIT_INT) {
				MAX_BIT_INT = opBitSize;
			}
		}
	}

	// Updates the Min and Max values.
	Min = APInt::getSignedMinValue(MAX_BIT_INT);
	Max = APInt::getSignedMaxValue(MAX_BIT_INT);
}

template <class CGT>
bool RangeAnalysis<CGT>::runOnFunction(Function &F) {
	getMaxBitWidth(F);

	// The data structures
	DenseMap<const Value*, VarNode*> VNodes;
	SmallPtrSet<BasicOp*, 64> GOprs;
	DenseMap<const Value*, SmallPtrSet<BasicOp*, 8> > UMap;
	DenseMap<const Value*, ValueBranchMap> VBMap;
	CGT CG(&VNodes, &GOprs, &UMap, &VBMap);

	// Build the graph and find the intervals of the variables.
	CG.buildGraph(F);
//	CG.print(F, errs());
	CG.printToFile(F,"/tmp/"+F.getName()+"cgpre.dot");
	CG.findIntervals(F);
	CG.printToFile(F,"/tmp/"+F.getName()+"cgpos.dot");
	return false;
}

template <class CGT>
void RangeAnalysis<CGT>::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
}


template <class CGT>
char RangeAnalysis<CGT>::ID = 0;

static RegisterPass<RangeAnalysis<ConstraintGraph> > Y("range-analysis", "Range Analysis with Cousot");
static RegisterPass<RangeAnalysis<CropDFS> > Z("range-analysis-crop", "Range Analysis with CropDFS");

// ========================================================================== //
// Range
// ========================================================================== //

Range::Range() : l(Min), u(Max), isEmpty(false) {}

Range::Range(APInt lb, APInt ub, bool isEmpty = false) :
	l(lb.sextOrTrunc(MAX_BIT_INT)),
	u(ub.sextOrTrunc(MAX_BIT_INT)),
	isEmpty(isEmpty) {}

Range::~Range() {}

bool Range::isMaxRange() const {
	return this->getLower().eq(Min) && this->getUpper().eq(Max);
}

/// Add and Mul are commutatives. So, they are a little different 
/// of the other operations.
Range Range::add(const Range& other) {
	if (this->isEmptySet() || other.isEmptySet()) {
		return Range(Min, Max, true);
	}

	APInt l = Min, u = Max;
	if (this->getLower().ne(Min) && other.getLower().ne(Min)) {
		l = getLower() + other.getLower();
	}

	if (this->getUpper().ne(Max) && other.getUpper().ne(Max)) {
		u = getUpper() + other.getUpper();
	}

	return Range(l, u);
}

/// [a, b] − [c, d] = 
/// [min (a − c, a − d, b − c, b − d), 
/// max (a − c, a − d, b − c, b − d)] = [a − d, b − c]
/// The other operations are just like this operation.
Range Range::sub(const Range& other) {

	if (this->isEmptySet()) {
		return Range(Min, Max, true);
	}

	if (other.isEmptySet()) {
		return Range(this->getLower(), this->getUpper(), true);
	}

	APInt ll = Min, lu = Min, ul = Max, uu = Max;
	if (this->getLower().ne(Min) && other.getLower().ne(Min)) {
		ll = this->getLower() - other.getLower(); // lower lower
	}

	if (this->getLower().ne(Min) && other.getUpper().ne(Max)) {
		lu = this->getLower() - other.getUpper(); // lower upper
	}

	if (this->getUpper().ne(Max) && other.getLower().ne(Min)) {
		ul = this->getUpper() - other.getLower(); // upper lower
	}

	if (this->getUpper().ne(Max) && other.getUpper().ne(Max)) {
		uu = this->getUpper() - other.getUpper(); // upper upper
	}

	APInt l = ll.slt(lu) ? ll : lu;
	APInt u = uu.sgt(ul) ? uu : ul;

	return Range(l, u, false);
}

/// Add and Mul are commutatives. So, they are a little different 
/// of the other operations.
Range Range::mul(const Range& other) {

	if (this->isEmptySet() || other.isEmptySet()) {
		return Range(Min, Max, true);
	}

	APInt l = Min, u = Max;

	if (this->getLower().ne(Min) && other.getLower().ne(Min)) {
		l = getLower() * other.getLower();
	}

	if (this->getUpper().ne(Max) && other.getUpper().ne(Max)) {
		u = getUpper() * other.getUpper();
	}

	return Range(l, u, false);
}

Range Range::udiv(const Range& other) {
	if (this->isEmptySet()) {
		return Range(Min, Max, true);
	}

	if (other.isEmptySet()) {
		return Range(this->getLower(), this->getUpper(), false);
	}

	APInt ll = Min, lu = Min, ul = Max, uu = Max;
	if (this->getLower().ne(Min) && other.getLower().ne(Min)) {
		if (other.getLower().ne(APInt::getNullValue(MAX_BIT_INT))) {
			ll = this->getLower().udiv(other.getLower()); // lower lower
		}
	}

	if (this->getLower().ne(Min) && other.getUpper().ne(Max)) {
		if (other.getUpper().ne(APInt::getNullValue(MAX_BIT_INT))) {
			lu = this->getLower().udiv(other.getUpper()); // lower upper
		}
	}

	if (this->getUpper().ne(Max) && other.getLower().ne(Min)) {
		if (other.getLower().ne(APInt::getNullValue(MAX_BIT_INT))) {
			ul = this->getUpper().udiv(other.getLower()); // upper lower
		}
	}

	if (this->getUpper().ne(Max) && other.getUpper().ne(Max)) {
		if (other.getUpper().ne(APInt::getNullValue(MAX_BIT_INT))) {
			uu = this->getUpper().udiv(other.getUpper()); // upper upper
		}
	}

	APInt l = ll.slt(lu) ? ll : lu;
	APInt u = uu.sgt(ul) ? uu : ul;

	return Range(l, u, false);

}

Range Range::sdiv(const Range& other) {
	if (this->isEmptySet()) {
		return Range(Min, Max, true);
	}

	if (other.isEmptySet()) {
		return Range(this->getLower(), this->getUpper(), false);
	}

	APInt ll = Min, lu = Min, ul = Max, uu = Max;
	if (this->getLower().ne(Min) && other.getLower().ne(Min)) {
		if (other.getLower().ne(APInt::getNullValue(MAX_BIT_INT))) {
			ll = this->getLower().sdiv(other.getLower()); // lower lower
		}
	}

	if (this->getLower().ne(Min) && other.getUpper().ne(Max)) {
		if (other.getUpper().ne(APInt::getNullValue(MAX_BIT_INT))) {
			lu = this->getLower().sdiv(other.getUpper()); // lower upper
		}
	}

	if (this->getUpper().ne(Max) && other.getLower().ne(Min)) {
		if (other.getLower().ne(APInt::getNullValue(MAX_BIT_INT))) {
			ul = this->getUpper().sdiv(other.getLower()); // upper lower
		}
	}

	if (this->getUpper().ne(Max) && other.getUpper().ne(Max)) {
		if (other.getUpper().ne(APInt::getNullValue(MAX_BIT_INT))) {
			uu = this->getUpper().sdiv(other.getUpper()); // upper upper
		}
	}

	APInt l = ll.slt(lu) ? ll : lu;
	APInt u = uu.sgt(ul) ? uu : ul;

	return Range(l, u, false);
}


Range Range::urem(const Range& other) {
	if (this->isEmptySet()) {
		return Range(Min, Max, true);
	}

	if (other.isEmptySet()) {
		return Range(this->getLower(), this->getUpper(), false);
	}

	APInt ll = Min, lu = Min, ul = Max, uu = Max;
	if (this->getLower().ne(Min) && other.getLower().ne(Min)) {
		ll = this->getLower().urem(other.getLower()); // lower lower
	}

	if (this->getLower().ne(Min) && other.getUpper().ne(Max)) {
		lu = this->getLower().urem(other.getUpper()); // lower upper
	}

	if (this->getUpper().ne(Max) && other.getLower().ne(Min)) {
		ul = this->getUpper().urem(other.getLower()); // upper lower
	}

	if (this->getUpper().ne(Max) && other.getUpper().ne(Max)) {
		uu = this->getUpper().urem(other.getUpper()); // upper upper
	}

	APInt l = ll.slt(lu) ? ll : lu;
	APInt u = uu.sgt(ul) ? uu : ul;

	return Range(l, u, false);
}


Range Range::srem(const Range& other) {
	if (this->isEmptySet()) {
		return Range(Min, Max, true);
	}

	if (other.isEmptySet()) {
		return Range(this->getLower(), this->getUpper(), false);
	}

	APInt ll = Min, lu = Min, ul = Max, uu = Max;
	if (this->getLower().ne(Min) && other.getLower().ne(Min)) {
		ll = this->getLower().srem(other.getLower()); // lower lower
	}

	if (this->getLower().ne(Min) && other.getUpper().ne(Max)) {
		lu = this->getLower().srem(other.getUpper()); // lower upper
	}

	if (this->getUpper().ne(Max) && other.getLower().ne(Min)) {
		ul = this->getUpper().srem(other.getLower()); // upper lower
	}

	if (this->getUpper().ne(Max) && other.getUpper().ne(Max)) {
		uu = this->getUpper().srem(other.getUpper()); // upper upper
	}

	APInt l = ll.slt(lu) ? ll : lu;
	APInt u = uu.sgt(ul) ? uu : ul;

	return Range(l, u, false);
}


Range Range::shl(const Range& other) {
	if (this->isEmptySet()) {
		return Range(Min, Max, true);
	}

	if (other.isEmptySet()) {
		return Range(this->getLower(), this->getUpper(), false);
	}

	APInt ll = Min, lu = Min, ul = Max, uu = Max;
	if (this->getLower().ne(Min) && other.getLower().ne(Min)) {
		ll = this->getLower().shl(other.getLower()); // lower lower
	}

	if (this->getLower().ne(Min) && other.getUpper().ne(Max)) {
		lu = this->getLower().shl(other.getUpper()); // lower upper
	}

	if (this->getUpper().ne(Max) && other.getLower().ne(Min)) {
		ul = this->getUpper().shl(other.getLower()); // upper lower
	}

	if (this->getUpper().ne(Max) && other.getUpper().ne(Max)) {
		uu = this->getUpper().shl(other.getUpper()); // upper upper
	}

	APInt l = ll.slt(lu) ? ll : lu;
	APInt u = uu.sgt(ul) ? uu : ul;

	return Range(l, u, false);

}


Range Range::lshr(const Range& other) {
	if (this->isEmptySet()) {
		return Range(Min, Max, true);
	}

	if (other.isEmptySet()) {
		return Range(this->getLower(), this->getUpper(), false);
	}

	APInt ll = Min, lu = Min, ul = Max, uu = Max;
	if (this->getLower().ne(Min) && other.getLower().ne(Min)) {
		ll = this->getLower().lshr(other.getLower()); // lower lower
	}

	if (this->getLower().ne(Min) && other.getUpper().ne(Max)) {
		lu = this->getLower().lshr(other.getUpper()); // lower upper
	}

	if (this->getUpper().ne(Max) && other.getLower().ne(Min)) {
		ul = this->getUpper().lshr(other.getLower()); // upper lower
	}

	if (this->getUpper().ne(Max) && other.getUpper().ne(Max)) {
		uu = this->getUpper().lshr(other.getUpper()); // upper upper
	}

	APInt l = ll.slt(lu) ? ll : lu;
	APInt u = uu.sgt(ul) ? uu : ul;

	return Range(l, u, false);

}


Range Range::ashr(const Range& other) {
	if (this->isEmptySet()) {
		return Range(Min, Max, true);
	}

	if (other.isEmptySet()) {
		return Range(this->getLower(), this->getUpper(), false);
	}

	APInt ll = Min, lu = Min, ul = Max, uu = Max;
	if (this->getLower().ne(Min) && other.getLower().ne(Min)) {
		ll = this->getLower().ashr(other.getLower()); // lower lower
	}

	if (this->getLower().ne(Min) && other.getUpper().ne(Max)) {
		lu = this->getLower().ashr(other.getUpper()); // lower upper
	}

	if (this->getUpper().ne(Max) && other.getLower().ne(Min)) {
		ul = this->getUpper().ashr(other.getLower()); // upper lower
	}

	if (this->getUpper().ne(Max) && other.getUpper().ne(Max)) {
		uu = this->getUpper().ashr(other.getUpper()); // upper upper
	}

	APInt l = ll.slt(lu) ? ll : lu;
	APInt u = uu.sgt(ul) ? uu : ul;

	return Range(l, u, false);

}


Range Range::And(const Range& other) {
	if (this->isEmptySet()) {
		return Range(Min, Max, true);
	}

	if (other.isEmptySet()) {
		return Range(this->getLower(), this->getUpper(), false);
	}

	APInt ll = Min, lu = Min, ul = Max, uu = Max;
	if (this->getLower().ne(Min) && other.getLower().ne(Min)) {
		ll = this->getLower().And(other.getLower()); // lower lower
	}

	if (this->getLower().ne(Min) && other.getUpper().ne(Max)) {
		lu = this->getLower().And(other.getUpper()); // lower upper
	}

	if (this->getUpper().ne(Max) && other.getLower().ne(Min)) {
		ul = this->getUpper().And(other.getLower()); // upper lower
	}

	if (this->getUpper().ne(Max) && other.getUpper().ne(Max)) {
		uu = this->getUpper().And(other.getUpper()); // upper upper
	}

	APInt l = ll.slt(lu) ? ll : lu;
	APInt u = uu.sgt(ul) ? uu : ul;

	return Range(l, u, false);

}


Range Range::Or(const Range& other) {
	if (this->isEmptySet()) {
		return Range(Min, Max, true);
	}

	if (other.isEmptySet()) {
		return Range(this->getLower(), this->getUpper(), false);
	}

	APInt ll = Min, lu = Min, ul = Max, uu = Max;
	if (this->getLower().ne(Min) && other.getLower().ne(Min)) {
		ll = this->getLower().Or(other.getLower()); // lower lower
	}

	if (this->getLower().ne(Min) && other.getUpper().ne(Max)) {
		lu = this->getLower().Or(other.getUpper()); // lower upper
	}

	if (this->getUpper().ne(Max) && other.getLower().ne(Min)) {
		ul = this->getUpper().Or(other.getLower()); // upper lower
	}

	if (this->getUpper().ne(Max) && other.getUpper().ne(Max)) {
		uu = this->getUpper().Or(other.getUpper()); // upper upper
	}

	APInt l = ll.slt(lu) ? ll : lu;
	APInt u = uu.sgt(ul) ? uu : ul;

	return Range(l, u, false);

}


Range Range::Xor(const Range& other) {
	if (this->isEmptySet()) {
		return Range(Min, Max, true);
	}

	if (other.isEmptySet()) {
		return Range(this->getLower(), this->getUpper(), false);
	}

	APInt ll = Min, lu = Min, ul = Max, uu = Max;
	if (this->getLower().ne(Min) && other.getLower().ne(Min)) {
		ll = this->getLower().Xor(other.getLower()); // lower lower
	}

	if (this->getLower().ne(Min) && other.getUpper().ne(Max)) {
		lu = this->getLower().Xor(other.getUpper()); // lower upper
	}

	if (this->getUpper().ne(Max) && other.getLower().ne(Min)) {
		ul = this->getUpper().Xor(other.getLower()); // upper lower
	}

	if (this->getUpper().ne(Max) && other.getUpper().ne(Max)) {
		uu = this->getUpper().Xor(other.getUpper()); // upper upper
	}

	APInt l = ll.slt(lu) ? ll : lu;
	APInt u = uu.sgt(ul) ? uu : ul;

	return Range(l, u, false);

}


Range Range::truncate(unsigned bitwidht) const {
	if (this->isEmptySet()) {
		return Range(APInt::getSignedMinValue(bitwidht),
				     APInt::getSignedMaxValue(bitwidht), true);
	}

	return Range(getLower().trunc(bitwidht),
			     getUpper().trunc(bitwidht),
			     isEmptySet());
}

Range Range::signExtend(unsigned bitwidht) const {
	if (isEmptySet()) {
		return Range(APInt::getSignedMinValue(bitwidht),
				     APInt::getSignedMaxValue(bitwidht), true);
	}
	
	return Range(getLower().sext(bitwidht), getUpper().sext(bitwidht), isEmptySet());
}

Range Range::zeroExtend(unsigned bitwidht) const {
	if (isEmptySet()) {
		return Range(APInt::getSignedMinValue(bitwidht),
				     APInt::getSignedMaxValue(bitwidht), true);
	}
	
	return Range(getLower().zext(bitwidht), getUpper().zext(bitwidht), isEmptySet());
}


Range Range::sextOrTrunc(unsigned bitwidht) const {
	if (this->isEmptySet()) {
		return Range(APInt::getSignedMinValue(bitwidht),
				     APInt::getSignedMaxValue(bitwidht), true);
	}

	// FIXME (maybe fixed)
	return signExtend(bitwidht);
}


Range Range::zextOrTrunc(unsigned bitwidht) const {
	if (this->isEmptySet()) {
		return Range(APInt::getSignedMinValue(bitwidht),
				     APInt::getSignedMaxValue(bitwidht), true);
	}

	// FIXME
	return Range(Min, Max, false);


}


Range Range::intersectWith(const Range& other) const {

	if (this->isEmptySet()) {
		return other;
	}

	if (other.isEmptySet()) {
		return Range(this->getLower(), this->getUpper(), this->isEmptySet());
	}

	APInt l = getLower().sgt(other.getLower()) ? getLower() : other.getLower();
	APInt u = getUpper().slt(other.getUpper()) ? getUpper() : other.getUpper();
	return Range(l, u, false);
}


Range Range::unionWith(const Range& other) const {
	if (this->isEmptySet()) {
		return other;
	}

	if (other.isEmptySet()) {
		return Range(this->getLower(), this->getUpper(), this->isEmptySet());
	}

	APInt l = getLower().slt(other.getLower()) ? getLower() : other.getLower();
	APInt u = getUpper().sgt(other.getUpper()) ? getUpper() : other.getUpper();
	return Range(l, u, false);
}


bool Range::operator==(const Range& other) const {
	return getLower().eq(other.getLower()) && getUpper().eq(other.getUpper());
}


bool Range::operator!=(const Range& other) const {
	return !(getLower().eq(other.getLower()) && getUpper().eq(other.getUpper()));
}


void Range::print(raw_ostream& OS) const {
	if (this->isEmptySet()) {
		OS << "empty-set";
		return;
	}

	if (getLower().eq(Min)) {
		OS << "[-inf, ";
	} else {
		OS << "[" << getLower() << ", ";
	}

	if (getUpper().eq(Max)) {
		OS << "+inf] ";
	} else {
		OS << getUpper() << "]";
	}
}

raw_ostream& operator<<(raw_ostream& OS, const Range& R) {
	R.print(OS);
	return OS;
}


// ========================================================================== //
// BasicInterval
// ========================================================================== //

BasicInterval::BasicInterval(const Range& range) : range(range) {}

BasicInterval::BasicInterval() : range(Range(Min, Max, false)) {}

BasicInterval::BasicInterval(const APInt& l, const APInt& u) : 
                             range(Range(l, u)) {}

// This is a base class, its dtor must be virtual.
BasicInterval::~BasicInterval() {}

/// Pretty print.
void BasicInterval::print(raw_ostream& OS) const {
	this->getRange().print(OS);
}


// ========================================================================== //
// SymbInterval
// ========================================================================== //

SymbInterval::SymbInterval(const Range& range,
						   const Value* bound,
						   CmpInst::Predicate pred) :
						   BasicInterval(range),
						   bound(bound),
						   pred(pred) {}

SymbInterval::~SymbInterval() {}


Range SymbInterval::fixIntersects(VarNode* bound, VarNode* sink) {
	// Get the lower and the upper bound of the
	// node which bounds this intersection.
	APInt l = bound->getRange().getLower();
	APInt u = bound->getRange().getUpper();

	// Get the lower and upper bound of the interval of this operation
	APInt lower = sink->getRange().getLower();
	APInt upper = sink->getRange().getUpper();

	switch (this->getOperation()) {
	case ICmpInst::ICMP_EQ: // equal
		return Range(l, u, false);
		break;
	case ICmpInst::ICMP_SLE: // signed less or equal
		return Range(lower, u + 1, false);
		break;
	case ICmpInst::ICMP_SLT: // signed less than
		return Range(lower, u, false);
		break;
	case ICmpInst::ICMP_SGE: // signed greater or equal
		return Range(l + 1, upper, false);
		break;
	case ICmpInst::ICMP_SGT: // signed greater than
		return Range(l, upper, false);
		break;
	default:
		return Range(Min, Max, false);
	}

	return Range(Min, Max, false);
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


// ========================================================================== //
// VarNode
// ========================================================================== //

/// The ctor.
VarNode::VarNode(const Value* V) : V(V), interval(Range(Min, Max, true)) {}

/// The dtor.
VarNode::~VarNode() {}


/// Initializes the value of the node.
void VarNode::init() {
	const Value* V = this->getValue();
	if (const ConstantInt *CI = dyn_cast<ConstantInt>(V)) {
		APInt tmp = CI->getValue();
		APInt value = tmp.sextOrTrunc(MAX_BIT_INT);
		this->setRange(Range(value, value, false));
	} else {
		// Initialize with a basic, empty, interval.
		this->setRange(Range(Min, Max, /*isEmpty=*/true));
	}
}


/// Pretty print.
void VarNode::print(raw_ostream& OS) const {
	if (const ConstantInt* C = dyn_cast<ConstantInt>(this->getValue())) {
		OS << C->getValue() << " ";
	} else {
		printVarName(this->getValue(), OS);
	}

	this->getRange().print(OS);
}

void VarNode::storeAbstractState(){
	if(this->interval.getLower().eq(Min))
		if(this->interval.getUpper().eq(Max))
			this->abstractState = '?';
		else
			this->abstractState = '-';
	else 
		if (this->interval.getLower().eq(Max))
			this->abstractState = '+';
		else
			this->abstractState = '0';
}


raw_ostream& operator<<(raw_ostream& OS, const VarNode* VN) {
	VN->print(OS);
	return OS;
}


// ========================================================================== //
// BasicOp
// ========================================================================== //

/// We can not want people creating objects of this class,
/// but we want to inherit of it.
BasicOp::BasicOp(BasicInterval* intersect, VarNode* sink, const Instruction *inst) :
        				 intersect(intersect),
				         sink(sink),
				         inst(inst) {}


/// We can not want people creating objects of this class,
/// but we want to inherit of it.
BasicOp::~BasicOp() {
	delete intersect;
}


/// Replace symbolic intervals with hard-wired constants.
void BasicOp::fixIntersects(VarNode* V) {
	if (SymbInterval* SI = dyn_cast<SymbInterval>(getIntersect())) {
		Range r = SI->fixIntersects(V, getSink());
		this->setIntersect(SI->fixIntersects(V, getSink()));
	}
}

// ========================================================================== //
// ControlDep
// ========================================================================== //

ControlDep::ControlDep(VarNode* sink, VarNode* source) :
          					   BasicOp(new BasicInterval(), sink, NULL),
					             source(source) {}

ControlDep::~ControlDep() {}

Range ControlDep::eval() const {
	return Range(Min, Max);
}
    
void ControlDep::print(raw_ostream& OS) const {}

// ========================================================================== //
// UnaryOp
// ========================================================================== //

UnaryOp::UnaryOp(BasicInterval* intersect,
        				 VarNode* sink,
        				 const Instruction *inst,
        				 VarNode* source,
        				 unsigned int opcode) :
        				 BasicOp(intersect, sink, inst),
        				 source(source),
        				 opcode(opcode) {}


// The dtor.
UnaryOp::~UnaryOp() {}

/// Computes the interval of the sink based on the interval of the sources,
/// the operation and the interval associated to the operation.
Range UnaryOp::eval() const {
	unsigned bw = getSink()->getValue()->getType()->getPrimitiveSizeInBits();
	Range result(Min, Max, false);
	switch (this->getOpcode()) {
	case Instruction::Trunc:
		result = source->getRange().truncate(bw);
		break;
	case Instruction::ZExt:
		result = source->getRange().zextOrTrunc(bw);
		break;
	case Instruction::SExt:
		result = source->getRange().sextOrTrunc(bw);
		break;
	default:
		// Phi functions, Loads and Stores are handled here.
		result = source->getRange();
		break;
	}

	if (!getIntersect()->getRange().isMaxRange()) {
		Range aux(getIntersect()->getRange());
		result = result.intersectWith(aux);
	}
	// To ensure that we always are dealing with the correct bit width.
	return result;
}


/// Prints the content of the operation. I didn't it an operator overload
/// because I had problems to access the members of the class outside it.
void UnaryOp::print(raw_ostream& OS) const {
	const char* quot = "\"";
	OS << " " << quot << this << quot << " [label =\"";
	
	this->getIntersect()->print(OS);
	OS << "\"]\n";
	
	const Value* V = this->getSource()->getValue();
	if (const ConstantInt* C = dyn_cast<ConstantInt>(V)) {
		OS << " " << C->getValue() << " -> " << quot << this << quot << "\n";
	} else {
		OS << " " << quot;
		printVarName(V, OS);
		OS << quot << " -> " << quot << this << quot << "\n";
	}

	const Value* VS = this->getSink()->getValue();
	OS << " " << quot << this << quot << " -> " << quot;
	printVarName(VS, OS);
	OS << quot << "\n";
}


// ========================================================================== //
// SigmaOp
// ========================================================================== //

SigmaOp::SigmaOp(BasicInterval* intersect,
        				 VarNode* sink,
        				 const Instruction *inst,
        				 VarNode* source,
        				 unsigned int opcode) :
        				 UnaryOp(intersect, sink, inst, source, opcode) {}


// The dtor.
SigmaOp::~SigmaOp() {}

/// Computes the interval of the sink based on the interval of the sources,
/// the operation and the interval associated to the operation.
Range SigmaOp::eval() const {
	Range result = this->getSource()->getRange();

	if (!getIntersect()->getRange().isMaxRange()) {
		result = result.intersectWith(getIntersect()->getRange());
	}

	return result;
}


/// Prints the content of the operation. I didn't it an operator overload
/// because I had problems to access the members of the class outside it.
void SigmaOp::print(raw_ostream& OS) const {
	const char* quot = "\"";
	OS << " " << quot << this << quot << " [label =\"";
	this->getIntersect()->print(OS);
	OS << "\"]\n";
	const Value* V = this->getSource()->getValue();
	if (const ConstantInt* C = dyn_cast<ConstantInt>(V)) {
		OS << " " << C->getValue() << " -> " << quot << this << quot << "\n";
	} else {
		OS << " " << quot;
		printVarName(V, OS);
		OS << quot << " -> " << quot << this << quot << "\n";
	}

	const Value* VS = this->getSink()->getValue();
	OS << " " << quot << this << quot << " -> "
	   << quot;
	printVarName(VS, OS);
	OS << quot << "\n";
}

// ========================================================================== //
// BinaryOp
// ========================================================================== //

// The ctor.
BinaryOp::BinaryOp(BasicInterval* intersect,
        				   VarNode* sink,
        				   const Instruction* inst,
        				   VarNode* source1,
        				   VarNode* source2,
        				   unsigned int opcode) :
        				   BasicOp(intersect, sink, inst),
        				   source1(source1),
        				   source2(source2),
        				   opcode(opcode) {}

/// The dtor.
BinaryOp::~BinaryOp() {}

/// Computes the interval of the sink based on the interval of the sources,
/// the operation and the interval associated to the operation.
/// Basically, this function performs the operation indicated in its opcode
/// taking as its operands the source1 and the source2.
Range BinaryOp::eval() const {
	Range op1 = this->getSource1()->getRange();
	Range op2 = this->getSource2()->getRange();
	Range result(Min, Max, /*is empty=*/true);

	switch (this->getOpcode()) {
	case Instruction::Add:
		result = op1.add(op2);
		break;
	case Instruction::Sub:
		result = op1.sub(op2);
		break;
	case Instruction::Mul:
		result = op1.mul(op2);
		break;
	case Instruction::UDiv:
		result = op1.udiv(op2);
		break;
	case Instruction::SDiv:
		result = op1.sdiv(op2);
		break;
	case Instruction::URem:
		result = op1.urem(op2);
		break;
	case Instruction::SRem:
		result = op1.srem(op2);
		break;
	case Instruction::Shl:
		result = op1.shl(op2);
		break;
	case Instruction::LShr:
		result = op1.lshr(op2);
		break;
	case Instruction::AShr:
		result = op1.ashr(op2);
		break;
	case Instruction::And:
		result = op1.And(op2);
		break;
	case Instruction::Or:
		result = op1.Or(op2);
		break;
	case Instruction::Xor:
		result = op1.Xor(op2);
		break;
	default:
		break;
	}

	if (!this->getIntersect()->getRange().isMaxRange()) {
		Range aux = this->getIntersect()->getRange();
		result = result.intersectWith(aux);
	}

	return result;
}



/// Pretty print.
void BinaryOp::print(raw_ostream& OS) const {
	const char* quot = "\"";
	const char* opcodeName = Instruction::getOpcodeName(this->getOpcode());
	OS << " " << quot << this << quot << " [label =\"" << opcodeName << "\"]\n";

	const Value* V1 = this->getSource1()->getValue();
	if (const ConstantInt* C = dyn_cast<ConstantInt>(V1)) {
		OS << " " << C->getValue() << " -> " << quot << this << quot << "\n";
	} else {
		OS << " " << quot;
		printVarName(V1, OS);
		OS << quot << " -> " << quot << this << quot << "\n";
	}

	const Value* V2 = this->getSource2()->getValue();
	if (const ConstantInt* C = dyn_cast<ConstantInt>(V2)) {
		OS << " " << C->getValue() << " -> " << quot << this << quot << "\n";
	} else {
		OS << " " << quot;
		printVarName(V2, OS);
		OS << quot << " -> " << quot << this << quot << "\n";
	}

	const Value* VS = this->getSink()->getValue();
	OS << " " << quot << this << quot << " -> "
	   << quot;
	printVarName(VS, OS);
	OS << quot << "\n";
}


// ========================================================================== //
// PhiOp
// ========================================================================== //


// The ctor.
PhiOp::PhiOp(BasicInterval* intersect,
			       VarNode* sink,
			       const Instruction* inst,
			       unsigned int opcode) :
			       BasicOp(intersect, sink, inst), opcode(opcode) {}

/// The dtor.
PhiOp::~PhiOp() {}

// Add source to the vector of sources
void PhiOp::addSource(const VarNode* newsrc)
{
	this->sources.push_back(newsrc);
}

/// Computes the interval of the sink based on the interval of the sources.
/// The result of evaluating a phi-function is the union of the ranges of
/// every variable used in the phi.
Range PhiOp::eval() const {
	Range result = this->getSource(0)->getRange();

	// Iterate over the sources of the phiop
	for (SmallVectorImpl<const VarNode*>::const_iterator sit = sources.begin()+1, send = sources.end(); sit != send; ++sit) {
		result = result.unionWith((*sit)->getRange());
	}

	return result;
}


/// Prints the content of the operation. I didn't it an operator overload
/// because I had problems to access the members of the class outside it.
void PhiOp::print(raw_ostream& OS) const {
	const char* quot = "\"";
	OS << " " << quot << this << quot << " [label =\"";
	OS << "phi";
	OS << "\"]\n";
	
	for (SmallVectorImpl<const VarNode*>::const_iterator sit = sources.begin(), send = sources.end(); sit != send; ++sit) {
		const Value* V = (*sit)->getValue();
		if (const ConstantInt* C = dyn_cast<ConstantInt>(V)) {
			OS << " " << C->getValue() << " -> " << quot << this << quot << "\n";
		} else {
			OS << " " << quot;
			printVarName(V, OS);
			OS << quot << " -> " << quot << this << quot << "\n";
		}
	}

	const Value* VS = this->getSink()->getValue();
	OS << " " << quot << this << quot << " -> "
	   << quot;
	printVarName(VS, OS);
	OS << quot << "\n";
}

// ========================================================================== //
// ValueBranchMap
// ========================================================================== //


ValueBranchMap::ValueBranchMap(const Value* V,
              							   const BasicBlock* BBTrue,
              							   const BasicBlock* BBFalse,
              							   BasicInterval* ItvT,
              							   BasicInterval* ItvF) :
              							   V(V), BBTrue(BBTrue),
              							   BBFalse(BBFalse), ItvT(ItvT), ItvF(ItvF) {}

ValueBranchMap::~ValueBranchMap() {}

void ValueBranchMap::clear() {
/*	if (ItvT) {
		delete ItvT;
		ItvT = NULL;
	}
	
	if (ItvF) {
		delete ItvF;
		ItvF = NULL;
	}
*/
}


// ========================================================================== //
// ConstraintGraph
// ========================================================================== //

ConstraintGraph::ConstraintGraph(VarNodes *varNodes, 
                                GenOprs *genOprs, 
                                UseMap *usemap,
                       					ValuesBranchMap *valuesbranchmap) {
	this->vars = varNodes;
	this->oprs = genOprs;
	this->useMap = usemap;
	this->valuesBranchMap = valuesbranchmap;
}

/// The dtor.
ConstraintGraph::~ConstraintGraph() {
	delete symbMap;
	
	for (VarNodes::iterator vit = vars->begin(), vend = vars->end(); vit != vend; ++vit) {
		delete vit->second;
	}
	
	for (GenOprs::iterator oit = oprs->begin(), oend = oprs->end(); oit != oend; ++oit) {
		delete *oit;
	}
}

/// Adds a VarNode to the graph.
VarNode* ConstraintGraph::addVarNode(const Value* V) {
	if (this->vars->count(V)) {
		return this->vars->find(V)->second;
	}

	VarNode* node = new VarNode(V);
	this->vars->insert(std::make_pair(V, node));

	// Inserts the node in the use map list.
	SmallPtrSet<BasicOp*, 8> useList;
	this->useMap->insert(std::make_pair(V, useList));
	return node;
}

/// Adds an UnaryOp in the graph.
void ConstraintGraph::addUnaryOp(const Instruction* I) {
	// Create the sink.
	VarNode* sink = addVarNode(I);
	// Create the source.
	VarNode* source = NULL;

	switch (I->getOpcode()) {
	case Instruction::Store:
		source = addVarNode(I->getOperand(1));
		break;
	case Instruction::Load:
	case Instruction::Trunc:
	case Instruction::ZExt:
	case Instruction::SExt:
		source = addVarNode(I->getOperand(0));
		break;
	default:
		return;
	}

	// Create the operation using the intersect to constrain sink's interval.
	UnaryOp* UOp = new UnaryOp(new BasicInterval(), sink, I, source, I->getOpcode());
	this->oprs->insert(UOp);

	// Inserts the sources of the operation in the use map list.
	this->useMap->find(source->getValue())->second.insert(UOp);
}

/// Adds an UnaryOp in the graph.
void ConstraintGraph::addUnaryOp(VarNode* sink, VarNode* source) {
	// Create the operation using the intersect to constrain sink's interval.
  // I'm using zero as the opcode because it doesn't matter here.
	UnaryOp* UOp = new UnaryOp(new BasicInterval(), sink, NULL, source, 0); 
	this->oprs->insert(UOp);

	// Inserts the sources of the operation in the use map list.
	this->useMap->find(source->getValue())->second.insert(UOp);
}


/// XXX: I'm assuming that we are always analyzing bytecodes in e-SSA form.
/// So, we don't have intersections associated with binary oprs. 
/// To have an intersect, we must have a Sigma instruction.
/// Adds a BinaryOp in the graph.
void ConstraintGraph::addBinaryOp(const Instruction* I) {
	// Create the sink.
	VarNode* sink = addVarNode(I);

	// Create the sources.
	VarNode *source1 = addVarNode(I->getOperand(0));
	VarNode *source2 = addVarNode(I->getOperand(1));

	// Create the operation using the intersect to constrain sink's interval.
	BasicInterval* BI = new BasicInterval();
	BinaryOp* BOp = new BinaryOp(BI, sink, I, source1, source2, I->getOpcode());

	// Insert the operation in the graph.
	this->oprs->insert(BOp);

	// Inserts the sources of the operation in the use map list.
	this->useMap->find(source1->getValue())->second.insert(BOp);
	this->useMap->find(source2->getValue())->second.insert(BOp);
}


/// Add a phi node (actual phi, does not include sigmas)
void ConstraintGraph::addPhiOp(const PHINode* Phi)
{
	// Create the sink.
	VarNode* sink = addVarNode(Phi);
	PhiOp* phiOp = new PhiOp(new BasicInterval(), sink, Phi, Phi->getOpcode());
	
	// Insert the operation in the graph.
	this->oprs->insert(phiOp);

	// Create the sources.
	for (User::const_op_iterator it = Phi->op_begin(), e = Phi->op_end(); it != e; ++it) {
		VarNode* source = addVarNode(*it);
		phiOp->addSource(source);
		
		// Inserts the sources of the operation in the use map list.
		this->useMap->find(source->getValue())->second.insert(phiOp);
	}
}

void ConstraintGraph::addSigmaOp(const PHINode* Sigma)
{
	// Create the sink.
	VarNode* sink = addVarNode(Sigma);
	BasicInterval* BItv = NULL;
	SigmaOp* sigmaOp = NULL;

	// Create the sources.
	for (User::const_op_iterator it = Sigma->op_begin(), e = Sigma->op_end(); it != e; ++it) {
		Value *operand = *it;
		VarNode* source = addVarNode(operand);
		// Create the operation.
		if (this->valuesBranchMap->count(operand)) {
			ValueBranchMap VBM = this->valuesBranchMap->find(operand)->second;
			if (Sigma->getParent() == VBM.getBBTrue()) {
				BItv = VBM.getItvT();
			} else {
				if (Sigma->getParent() == VBM.getBBFalse()) {
					BItv = VBM.getItvF();
				}
			}
		}

		if (BItv == NULL) {
			sigmaOp = new SigmaOp(new BasicInterval(), sink, Sigma, source, Sigma->getOpcode());
		} else {
			sigmaOp = new SigmaOp(BItv, sink, Sigma, source, Sigma->getOpcode());
		}

		// Insert the operation in the graph.
		this->oprs->insert(sigmaOp);

		// Inserts the sources of the operation in the use map list.
		this->useMap->find(source->getValue())->second.insert(sigmaOp);
	}
}

void ConstraintGraph::buildOperations(const Instruction* I) {
	// Handle binary instructions.
	if (I->isBinaryOp()) {
		addBinaryOp(I);
	} else {
		// Handle Phi functions.
		if (const PHINode* Phi = dyn_cast<PHINode>(I)) {
			if (Phi->getName().startswith(sigmaString)) {
				addSigmaOp(Phi);
			} else {
				addPhiOp(Phi);
			}
		} else {
			// We have an unary instruction to handle.
			addUnaryOp(I);
		}
	}
}

void ConstraintGraph::buildValueBranchMap(const Function& F) {
	// Fix the intersects using the ranges obtained in the branches.
	for (Function::const_iterator iBB = F.begin(), eBB = F.end(); iBB != eBB; ++iBB){
		const TerminatorInst* ti = iBB->getTerminator();
		if(!isa<BranchInst>(ti))	continue;
		const BranchInst * br = cast<BranchInst>(ti);
		if(!br->isConditional())	continue;
		ICmpInst *ici = dyn_cast<ICmpInst> (br->getCondition());
		if(!ici) continue;

		//FIXME: handle switch case later
		const Type* op0Type = ici->getOperand(0)->getType();
		const Type* op1Type = ici->getOperand(1)->getType();
		if (!op0Type->isIntegerTy() || !op1Type->isIntegerTy()) {
			continue;
		}

		// Gets the successors of the current basic block.
		const BasicBlock *TBlock = br->getSuccessor(0);
		const BasicBlock *FBlock = br->getSuccessor(1);

		// We have a Variable-Constant comparison.
		if (ConstantInt *CI = dyn_cast<ConstantInt>(ici->getOperand(1))) {
			// Calculate the range of values that would satisfy the comparison.
			ConstantRange CR(CI->getValue(), CI->getValue() + 1);
			unsigned int pred = ici->getPredicate();
			
			// TODO: Fix this interval to saturate on sums
			ConstantRange tmpT = ConstantRange::makeICmpRegion(pred, CR);
			APInt sigMin = tmpT.getSignedMin();
			APInt sigMax = tmpT.getSignedMax();

			if (sigMax.slt(sigMin)) {
				sigMax = APInt::getSignedMaxValue(MAX_BIT_INT);
			}
			
			Range TValues = Range(sigMin, sigMax, false);

			// If we're interested in the false dest, invert the condition.
			ConstantRange tmpF = tmpT.inverse();
			sigMin = tmpF.getSignedMin();
			sigMax = tmpF.getSignedMax();
			
			if (sigMax.slt(sigMin)) {
				sigMax = APInt::getSignedMaxValue(MAX_BIT_INT);
			}
			
			Range FValues = Range(sigMin, sigMax, false);

			// Create the interval using the intersection in the branch.
			BasicInterval* BT = new BasicInterval(TValues);
			BasicInterval* BF = new BasicInterval(FValues);
			ValueBranchMap VBM(ici->getOperand(0), TBlock, FBlock, BT, BF);
			valuesBranchMap->insert(std::make_pair(ici->getOperand(0), VBM));
		} else {
			// Create the interval using the intersection in the branch.
			CmpInst::Predicate pred = ici->getPredicate();
			CmpInst::Predicate invPred = ici->getInversePredicate();
			Range CR(Min, Max, false);
			const Value* Op1 = ici->getOperand(1);

			// Symbolic intervals for op0
			const Value* Op0 = ici->getOperand(0);
			SymbInterval* STOp0 = new SymbInterval(CR, Op1, pred);
			SymbInterval* SFOp0 = new SymbInterval(CR, Op1, invPred);
			ValueBranchMap VBMOp0(Op0, TBlock, FBlock, STOp0, SFOp0);
			valuesBranchMap->insert(std::make_pair(ici->getOperand(0), VBMOp0));

			// Symbolic intervals for op1
			SymbInterval* STOp1 = new SymbInterval(CR, Op0, invPred);
			SymbInterval* SFOp1 = new SymbInterval(CR, Op0, pred);
			ValueBranchMap VBMOp1(Op1, TBlock, FBlock, STOp1, SFOp1);
			valuesBranchMap->insert(std::make_pair(ici->getOperand(1), VBMOp1));
		}
	}
}

/// Iterates through all instructions in the function and builds the graph.
void ConstraintGraph::buildGraph(const Function& F) {
	buildValueBranchMap(F);
	for (const_inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
		const Type* ty = (&*I)->getType();
		if (!(ty->isIntegerTy() || ty->isPointerTy() || ty->isVoidTy())) {
			continue;
		}

		if (!isValidInstruction(&*I)) {
			continue;
		}

		buildOperations(&*I);
	}

	// Initializes the nodes and the use map structure.
	VarNodes::iterator bgn = this->vars->begin(), end = this->vars->end();
	for (; bgn != end; ++bgn) {
		bgn->second->init();
	}
}


/// This is the meet operator of the growth analysis. The growth analysis
/// will change the bounds of each variable, if necessary. Initially, each
/// variable is bound to either the undefined interval, e.g. [., .], or to
/// a constant interval, e.g., [3, 15]. After this analysis runs, there will
/// be no undefined interval. Each variable will be either bound to a
/// constant interval, or to [-, c], or to [c, +], or to [-, +].
bool Meet::widen(BasicOp* op) {

	Range oldInterval = op->getSink()->getRange();
	Range newInterval = op->eval();

	APInt oldLower = oldInterval.getLower();
	APInt oldUpper = oldInterval.getUpper();
	APInt newLower = newInterval.getLower();
	APInt newUpper = newInterval.getUpper();

	if (oldInterval.isEmptySet()) {
		op->getSink()->setRange(newInterval);
	} else {
		if (newLower.slt(oldLower) && newUpper.sgt(oldUpper)) {
			op->getSink()->setRange(Range(Min, Max, false));
		} else {
			if (newLower.slt(oldLower)) {
				op->getSink()->setRange(Range(Min, oldUpper, false));
			} else {
				if (newUpper.sgt(oldUpper)) {
					op->getSink()->setRange(Range(oldLower, Max, false));
				}
			}
		}
	}

	Range sinkInterval = op->getSink()->getRange();
	return oldInterval != sinkInterval;
}

bool Meet::crop(BasicOp* op){
	Range oldInterval = op->getSink()->getRange();
	Range newInterval = op->eval();
	
	bool hasChanged = false;
	char abstractState = op->getSink()->getAbstractState();
	if((abstractState == '-' || abstractState == '?') && newInterval.getLower().sgt(oldInterval.getLower())){
		op->getSink()->setRange(Range(newInterval.getLower(), oldInterval.getUpper(), false));
		hasChanged = true;
	}
	
	if((abstractState == '+' || abstractState == '?') && newInterval.getUpper().slt(oldInterval.getUpper())){
		op->getSink()->setRange(Range(op->getSink()->getRange().getLower(), newInterval.getUpper(), false));
		hasChanged = true;
	}
	
	return hasChanged;
}

bool Meet::growth(BasicOp* op){
	Range oldInterval = op->getSink()->getRange();
	Range newInterval = op->eval();
	if (oldInterval.isEmptySet())
		op->getSink()->setRange(newInterval);
	else{
		APInt oldLower = oldInterval.getLower();
		APInt oldUpper = oldInterval.getUpper();
		APInt newLower = newInterval.getLower();
		APInt newUpper = newInterval.getUpper();
		if(newLower.slt(oldLower))
			if(newUpper.sgt(oldUpper))
				op->getSink()->setRange(Range());
			else
				op->getSink()->setRange(Range(Min,oldLower));
		else 
			if(newUpper.sgt(oldUpper))
				op->getSink()->setRange(Range(oldUpper,Max));
	}
	Range sinkInterval = op->getSink()->getRange();
	return oldInterval != sinkInterval;
}

/// This is the meet operator of the cropping analysis. Whereas the growth
/// analysis expands the bounds of each variable, regardless of intersections
/// in the constraint graph, the cropping analysis shyrinks these bounds back
/// to ranges that respect the intersections.
bool Meet::narrow(BasicOp* op) {
	APInt oLower = op->getSink()->getRange().getLower();
	APInt oUpper = op->getSink()->getRange().getUpper();
	Range newInterval = op->eval();

	APInt nLower = newInterval.getLower();
	APInt nUpper = newInterval.getUpper();
	bool hasChanged = false;

	if (oLower.eq(Min) && nLower.ne(Min)) {
		op->getSink()->setRange(Range(nLower, oUpper, false));
		hasChanged = true;
	} else {
		APInt smin = APIntOps::smin(oLower, nLower);
		if (oLower.ne(smin)) {
			op->getSink()->setRange(Range(smin, oUpper, false));
			hasChanged = true;
		}
	}

	if (oUpper.eq(Max) && nUpper.ne(Max)) {
		op->getSink()->setRange(Range(oLower, nUpper, false));
		hasChanged = true;
	} else {
		APInt smax = APIntOps::smax(oUpper, nUpper);
		if (oUpper.ne(smax)) {
			op->getSink()->setRange(Range(oLower, smax, false));
			hasChanged = true;
		}
	}

	return hasChanged;
}


void ConstraintGraph::update(const UseMap &compUseMap, SmallPtrSet<const Value*, 6>& actv, bool (*meet)(BasicOp* op)) {
	/*
	if (actv.empty()) {
		return;
	}

	const Value* V = *actv.begin();
	actv.erase(V);

	// The use list.
	SmallPtrSet<BasicOp*, 8> L = compUseMap.find(V)->second;
	SmallPtrSet<BasicOp*, 8>::iterator bgn = L.begin(),	end = L.end();
	for (; bgn != end; ++bgn) {
		if (meet(*bgn)) {
			// I want to use it as a set, but I also want
			// keep an order or insertions and removals.
			if (!actv.count((*bgn)->getSink()->getValue())) {
				actv.insert((*bgn)->getSink()->getValue());
			}

		}
	}

	update(compUseMap, actv, meet);
	*/
	
	
	while (!actv.empty()) {
		const Value* V = *actv.begin();
		actv.erase(V);
		
		// The use list.
		const SmallPtrSet<BasicOp*, 8> &L = compUseMap.find(V)->second;
		SmallPtrSetIterator<BasicOp*> bgn = L.begin(), end = L.end();
		
		for (; bgn != end; ++bgn) {
			if (meet(*bgn)) {
				// I want to use it as a set, but I also want
				// keep an order or insertions and removals.
				actv.insert((*bgn)->getSink()->getValue());
			}
		}
	}
	
}


/// Finds the intervals of the variables in the graph.
void ConstraintGraph::findIntervals() {

	// Builds symbMap
	buildSymbolicIntersectMap();
	
	// List of SCCs
	Nuutila sccList(vars, useMap, symbMap);
	
	errs() << "Number of strongly connected components: " << sccList.worklist.size() << "\n";
	
	// For each SCC in graph, do the following
	for (Nuutila::iterator nit = sccList.begin(), nend = sccList.end(); nit != nend; ++nit) {
		SmallPtrSet<VarNode*, 32> &component = sccList.components[*nit];
		
		errs() << component.size() << "\n";

		// DEBUG
		/*
		if (component.size() == 16) {
			std::string errors;
			std::string filename = "16";
			filename += ".dot";

			raw_fd_ostream output(filename.c_str(), errors);
			
			const char* quot = "\"";
			// Print the header of the .dot file.
			output << "digraph dotgraph {\n";
			output << "label=\"Constraint Graph for \"\n";
			output << "node [shape=record,fontname=\"Times-Roman\",fontsize=14];\n";

			// Print the body of the .dot file.
			for (SmallPtrSetIterator<VarNode*> bgn = component.begin(), end = component.end(); bgn != end; ++bgn) {
				if (const ConstantInt* C = dyn_cast<ConstantInt>((*bgn)->getValue())) {
					output << " " << C->getValue();
				} else {
					output << quot;
					printVarName((*bgn)->getValue(), output);
					output << quot;
				}

				output << " [label=\"";
				(*bgn)->print(output);
				output << " \"]\n";
			}
			
			for (SmallPtrSetIterator<VarNode*> bgn = component.begin(), end = component.end(); bgn != end; ++bgn) {
				VarNode *var = *bgn;
				
				SmallPtrSet<BasicOp*, 8> &uselist = (*this->useMap)[var->getValue()];
				
				for (SmallPtrSetIterator<BasicOp*> useit = uselist.begin(), usend = uselist.end(); useit != usend; ++useit) {
					if (component.count((*useit)->getSink())) {
						(*useit)->print(output);
						output << "\n";
					}
				}
			}			
			
			//output << pseudoEdgesString.str();

			// Print the footer of the .dot file.
			output << "}\n";

			output.close();
			
		}
		*/
		
		
		
		
		for (SmallPtrSetIterator<VarNode*> p = component.begin(), pend = component.end(); p != pend; ++p) {
			const ConstantInt *CI = NULL;
			
			if ((CI = dyn_cast<ConstantInt>((*p)->getValue()))) {
				errs() << CI->getValue() << "\n";
			}
			else {
				errs() << (*p)->getValue()->getName() << "\n";
			}
		}
		
		
		UseMap compUseMap = buildUseMap(component);
		
		// Get the entry points of the SCC
		SmallPtrSet<const Value*, 6> entryPoints;
		generateEntryPoints(component, entryPoints);
		// Primeiro iterate till fix point
		update(compUseMap, entryPoints, Meet::widen);
		fixIntersects(component);
		// Segundo iterate till fix point
		SmallPtrSet<const Value*, 6> activeVars;
		generateActivesVars(component, activeVars);
		update(compUseMap, activeVars, Meet::narrow);
		
		// TODO: PROPAGAR PARA O PROXIMO SCC
		propagateToNextSCC(component);
		
		printResultIntervals();
	}
	
	computeStats();
}

void ConstraintGraph::generateEntryPoints(SmallPtrSet<VarNode*, 32> &component, SmallPtrSet<const Value*, 6> &entryPoints){
	if (!entryPoints.empty()) {
		errs() << "Set não vazio\n";
	}
	
	// Iterate over the varnodes in the component
	for (SmallPtrSetIterator<VarNode*> cit = component.begin(), cend = component.end(); cit != cend; ++cit) {
		VarNode *var = *cit;
		const Value *V = var->getValue();
		
		// TODO: Verificar a condição para ser um entry point
		if (!var->getRange().isEmptySet()) {
			entryPoints.insert(V);
		}
	}
}

void ConstraintGraph::generateEntryPoints(SmallPtrSet<const Value*, 6> &entryPoints){
	if (!entryPoints.empty()) {
		errs() << "Set não vazio\n";
	}
	
	VarNodes::iterator vbgn = this->vars->begin(), vend = this->vars->end();
	for (; vbgn != vend; ++vbgn) {
		vbgn->second->init();
		const ConstantInt* CI = dyn_cast<ConstantInt>(vbgn->second->getValue());
		if (CI) {
			entryPoints.insert(vbgn->first);
		}
	}
}

void ConstraintGraph::fixIntersects(SmallPtrSet<VarNode*, 32> &component){
	// Iterate again over the varnodes in the component
	for (SmallPtrSetIterator<VarNode*> cit = component.begin(), cend = component.end(); cit != cend; ++cit) {
		VarNode *var = *cit;
		const Value *V = var->getValue();
		
		SymbMap::iterator sit = symbMap->find(V);
		
		if (sit != symbMap->end()) {
			for (SmallPtrSetIterator<BasicOp*> opit = sit->second.begin(), opend = sit->second.end(); opit != opend; ++opit) {
				BasicOp *op = *opit;
				
				op->fixIntersects(var);
			}
		}
	}
}

void ConstraintGraph::generateActivesVars(SmallPtrSet<VarNode*, 32> &component, SmallPtrSet<const Value*, 6> &activeVars){
	if (!activeVars.empty()) {
		errs() << "Set não vazio\n";
	}
	
	for (SmallPtrSetIterator<VarNode*> cit = component.begin(), cend = component.end(); cit != cend; ++cit) {
		VarNode *var = *cit;
		const Value *V = var->getValue();
		
		const ConstantInt* CI = dyn_cast<ConstantInt>(V);
		if (CI) {
			continue;
		}
		
		activeVars.insert(V);
	}
}

void ConstraintGraph::generateActivesVars(SmallPtrSet<const Value*, 6> &activeVars){
	if (!activeVars.empty()) {
		errs() << "Set não vazio\n";
	}

	for (VarNodes::iterator vbgn = vars->begin(), vend = vars->end(); vbgn != vend; ++vbgn) {
		const ConstantInt* CI = dyn_cast<ConstantInt>(vbgn->first);
		if (CI) {
			continue;
		}

		activeVars.insert(vbgn->first);
	}
}

void ConstraintGraph::findIntervals(const Function& F) {
	// Builds symbMap
//	buildSymbolicIntersectMap();
//	
//	// Get the entry points of the SCC
//	SmallPtrSet<const Value*, 6> entryPoints;
//	generateEntryPoints(entryPoints);

//	// Fernando's findInterval method
//	update(entryPoints, Meet::widen);
//	
//	errs() << "==========================Widen============================\n";
//	for (vbgn = vars->begin(), vend = vars->end(); vbgn != vend; ++vbgn) {
//		errs() << vbgn->first->getName() << " ";
//		vbgn->second->getRange().print(errs());
//		errs() << "\n";
//	}
//	errs() << "===========================================================\n";

//	GenOprs::iterator obgn = oprs->begin(), oend = oprs->end();
//	for (; obgn != oend; ++obgn) {

//		if (SymbInterval* SI = dyn_cast<SymbInterval>((*obgn)->getIntersect())) {
//			if (!this->vars->count(SI->getBound())) {
//				continue;
//			}

//			(*obgn)->fixIntersects(this->vars->find(SI->getBound())->second);
//		}
//	}

//	SmallPtrSet<const Value*, 6> activeVars;
//	generateActivesVars(activeVars);
//	update(activeVars, Meet::narrow);

//	errs() << "======================Narrow===============================\n";
//	for (vbgn = vars->begin(), vend = vars->end(); vbgn != vend; ++vbgn) {
//		errs() << vbgn->first->getName() << " ";
//		vbgn->second->getRange().print(errs());
//		errs() << "\n";
//	}
//	errs() << "===========================================================\n";


	// ==================================== //
	// Get the stats
	// ==================================== //
//	computeStats();
}

// TODO: To implement it.
/// Releases the memory used by the graph.
void ConstraintGraph::clear() {

}

/// Prints the content of the graph in dot format. For more informations
/// about the dot format, see: http://www.graphviz.org/pdf/dotguide.pdf
void ConstraintGraph::print(const Function& F, raw_ostream& OS) const {
	const char* quot = "\"";
	// Print the header of the .dot file.
	OS << "digraph dotgraph {\n";
	OS << "label=\"Constraint Graph for \'";
	OS << F.getNameStr() << "\' function\";\n";
	OS << "node [shape=record,fontname=\"Times-Roman\",fontsize=14];\n";

	// Print the body of the .dot file.
	VarNodes::const_iterator bgn, end;
	for (bgn = vars->begin(), end = vars->end(); bgn != end; ++bgn) {
		if (const ConstantInt* C = dyn_cast<ConstantInt>(bgn->first)) {
			OS << " " << C->getValue();
		} else {
			OS << quot;
			printVarName(bgn->first, OS);
			OS << quot;
		}

		OS << " [label=\"";
		bgn->second->print(OS);
		OS << " \"]\n";
	}

	GenOprs::const_iterator B = oprs->begin(), E = oprs->end();
	for (; B != E; ++B) {
		(*B)->print(OS);
		OS << "\n";
	}
	
	OS << pseudoEdgesString.str();

	// Print the footer of the .dot file.
	OS << "}\n";
}

void ConstraintGraph::printToFile(const Function& F, Twine FileName){
	std::string ErrorInfo;
	raw_fd_ostream file(FileName.str().c_str(), ErrorInfo);
	print(F,file);
	file.close();
}

void ConstraintGraph::printResultIntervals(){
	for (VarNodes::iterator vbgn = vars->begin(), vend = vars->end(); vbgn != vend; ++vbgn) {
		if (const ConstantInt* C = dyn_cast<ConstantInt>(vbgn->first)) {
			errs() << C->getValue() << " ";
		} else {
			printVarName(vbgn->first, errs());
		}
		
		vbgn->second->getRange().print(errs());
		errs() << "\n";
	}

	errs() << "\n";
}

void ConstraintGraph::computeStats(){
	for (VarNodes::iterator vbgn = vars->begin(), vend = vars->end(); vbgn != vend; ++vbgn) {
		// We only count the instructions that have uses.
		if (vbgn->first->getNumUses() == 0) {
			continue;
		}

		unsigned total = vbgn->first->getType()->getPrimitiveSizeInBits();
		usedBits += total;
		Range CR = vbgn->second->getRange();
		unsigned lb = CR.getLower().getActiveBits();
		unsigned ub = CR.getUpper().getActiveBits();
		unsigned nBits = lb > ub ? lb : ub;

		if ((lb != 0 || ub != 0) && nBits < total) {
			needBits += nBits;
		} else {
			needBits += total;
		}
	}

	double totalB = usedBits;
	double needB = needBits;
	double reduction = (double) (totalB - needB) * 100 / totalB;
	percentReduction = (unsigned int) reduction;
}

/*
 *	This method builds a map that binds each variable to the operation in
 *  which this variable is defined.
 */
/*
DefMap ConstraintGraph::buildDefMap(const std::deque<VarNode*> &component)
{
	std::deque<BasicOp*> list;
	for (GenOprs::iterator opit = oprs->begin(), opend = oprs->end(); opit != opend; ++opit) {
		BasicOp *op = *opit;
		
		if (std::find(component.begin(), component.end(), op->getSink()) != component.end()) {
			list.push_back(op);
		}
	}
	
	DefMap defMap;
	
	for (std::deque<BasicOp*>::iterator opit = list.begin(), opend = list.end(); opit != opend; ++opit) {
		BasicOp *op = *opit;
		defMap[op->getSink()] = op;
	}
	
	return defMap;
}
*/

/*
 *	This method builds a map that binds each variable label to the operations
 *  where this variable is used.
 */
UseMap ConstraintGraph::buildUseMap(const SmallPtrSet<VarNode*, 32> &component)
{
	UseMap compUseMap;
	
	for (SmallPtrSetIterator<VarNode*> vit = component.begin(), vend = component.end(); vit != vend; ++vit) {
		const VarNode *var = *vit;
		const Value *V = var->getValue();
		
		// Get the component's use list for V (it does not exist until we try to get it)
		SmallPtrSet<BasicOp*, 8> &list = compUseMap[V];
		
		// Get the use list of the variable in component
		UseMap::iterator p = this->useMap->find(V);
		
		// For each operation in the list, verify if its sink is in the component
		for (SmallPtrSetIterator<BasicOp*> opit = p->second.begin(), opend = p->second.end(); opit != opend; ++opit) {
			VarNode *sink = (*opit)->getSink();
			
			// If it is, add op to the component's use map
			if (component.count(sink)) {
				list.insert(*opit);
			}
		}
	}
	
	return compUseMap;
}

/*
 *	This method builds a map of variables to the lists of operations where
 *  these variables are used as futures. Its C++ type should be something like
 *  map<VarNode, List<Operation>>.
 */
void ConstraintGraph::buildSymbolicIntersectMap()
{
	// Creates the symbolic intervals map
	symbMap = new SymbMap();
	
	// Iterate over the operations set
	for(GenOprs::iterator oit = oprs->begin(), oend = oprs->end(); oit != oend; ++oit) {
		BasicOp *op = *oit;
		
		// If the operation is unary and its interval is symbolic
		UnaryOp *uop = dyn_cast<UnaryOp>(op);
		
		if (uop && isa<SymbInterval>(uop->getIntersect())) {
			SymbInterval* symbi = cast<SymbInterval>(uop->getIntersect());
			
			const Value *V = symbi->getBound();
			SymbMap::iterator p = symbMap->find(V);
			
			if (p != symbMap->end()) {
				p->second.insert(uop);
			}
			else {
				SmallPtrSet<BasicOp*, 8> l;
				
				if (!l.empty()) {
					errs() << "Erro no set\n";
				}
				
				l.insert(uop);
				symbMap->insert(std::make_pair(V, l));
			}
		}
		
		
	}
}

/*
 *	This method evaluates once each operation that uses a variable in
 *  component, so that the next SCCs after component will have entry
 *  points to kick start the range analysis algorithm.
 */
void ConstraintGraph::propagateToNextSCC(const SmallPtrSet<VarNode*, 32> &component)
{
	for (SmallPtrSetIterator<VarNode*> cit = component.begin(), cend = component.end(); cit != cend; ++cit) {
		VarNode *var = *cit;
		const Value *V = var->getValue();
		
		UseMap::iterator p = this->useMap->find(V);
		
		for (SmallPtrSetIterator<BasicOp*> sit = p->second.begin(), send = p->second.end(); sit != send; ++sit) {
			BasicOp *op = *sit;
			
			op->getSink()->setRange(op->eval());
		}
	}
}

/*
 *	Adds the edges that ensure that we solve a future before fixing its
 *  interval. I have created a new class: ControlDep edges, to represent
 *  the control dependences. In this way, in order to delete these edges,
 *  one just need to go over the map of uses removing every instance of the
 *  ControlDep class.
 */
void Nuutila::addControlDependenceEdges(SymbMap *symbMap, UseMap *useMap, VarNodes* vars)
{
	for (SymbMap::iterator sit = symbMap->begin(), send = symbMap->end(); sit != send; ++sit) {
		for (SmallPtrSetIterator<BasicOp*> opit = sit->second.begin(), opend = sit->second.end(); opit != opend; ++opit) {
			// Pega o intervalo
			SymbInterval* interval = cast<SymbInterval>((*opit)->getIntersect());
			
			// Cria uma operação pseudo-aresta
			VarNode* source = vars->find(interval->getBound())->second;
			
			if (source == NULL) {
				continue;
			}
			
			BasicOp *cdedge = new ControlDep(source, (*opit)->getSink());
			
			(*useMap)[(*opit)->getSink()->getValue()].insert(cdedge);
		}
	}
}

/*
 *	Removes the control dependence edges from the constraint graph.
 */
void Nuutila::delControlDependenceEdges(UseMap *useMap)
{
	for (UseMap::iterator it = useMap->begin(), end = useMap->end(); it != end; ++it) {
		
		std::deque<ControlDep*> ops;
		
		if (!ops.empty()) {
			errs() << "Erro na lista\n";
		}
		
		for (SmallPtrSetIterator<BasicOp*> sit = it->second.begin(), send = it->second.end(); sit != send; ++sit) {
			ControlDep *op = NULL;
			
			if ((op = dyn_cast<ControlDep>(*sit))) {
				ops.push_back(op);
			}
		}
		
		for (std::deque<ControlDep*>::iterator dit = ops.begin(), dend = ops.end(); dit != dend; ++dit) {
			ControlDep *op = *dit;
			
			
			// Add pseudo edge to the string
			const Value* V = op->getSource()->getValue();
			if (const ConstantInt* C = dyn_cast<ConstantInt>(V)) {
				pseudoEdgesString << " " << C->getValue() << " -> ";
			} else {
				pseudoEdgesString << " " << '"';
				printVarName(V, pseudoEdgesString);
				pseudoEdgesString << '"' << " -> ";
			}

			const Value* VS = op->getSink()->getValue();
			pseudoEdgesString << '"';
			printVarName(VS, pseudoEdgesString);
			pseudoEdgesString << '"';
			
			pseudoEdgesString << " [style=dashed]\n";
			
			
			// Remove pseudo edge from the map
			it->second.erase(op);
		}
	}
}

/*
 *	Finds SCCs using Nuutila's algorithm. This algorithm is divided in
 *  two parts. The first calls the recursive visit procedure on every node
 *  in the constraint graph. The second phase revisits these nodes,
 *  grouping them in components.
 */
void Nuutila::visit(Value *V, std::stack<Value*> &stack, UseMap *useMap)
{
	dfs[V] = index;
	++index;
	root[V] = V;

	// Visit every node defined in an instruction that uses V	
	for (SmallPtrSetIterator<BasicOp*> sit = (*useMap)[V].begin(), send = (*useMap)[V].end(); sit != send; ++sit) {
		BasicOp *op = *sit;
		Value *name = const_cast<Value*>(op->getSink()->getValue());
		
		if (dfs[name] < 0) {
			visit(name, stack, useMap);
		}
		
		if ((inComponent.count(name) == false) && (dfs[root[V]] >= dfs[root[name]])) {
			root[V] = root[name];
		}
	}
	
	// The second phase of the algorithm assigns components to stacked nodes
	if (root[V] == V) {
		// Neither the worklist nor the map of components is part of Nuutila's
		// original algorithm. We are using these data structures to get a
		// topological ordering of the SCCs without having to go over the root
		// list once more.
		worklist.push_back(V);
		
		SmallPtrSet<VarNode*, 32> SCC;
		
		if (!SCC.empty()) {
			errs() << "Erro na lista\n";
		}
		
		SCC.insert((*variables)[V]);
		
		inComponent.insert(V);
		
		while (!stack.empty() && (dfs[stack.top()] > dfs[V])) {
			Value *node = stack.top();
			stack.pop();
			
			inComponent.insert(node);
			
			SCC.insert((*variables)[node]);
		}
		
		components[V] = SCC;
	}
	else {
		stack.push(V);
	}
}

/*
 *	Finds the strongly connected components in the constraint graph formed by
 *	Variables and UseMap. The class receives the map of futures to insert the
 *  control dependence edges in the contraint graph. These edges are removed
 *  after the class is done computing the SCCs.
 */
Nuutila::Nuutila(VarNodes *varNodes, UseMap *useMap, SymbMap *symbMap, bool single)
{

if (single) {
	/* FERNANDO */
	SmallPtrSet<VarNode*, 32> SCC;
	for (VarNodes::iterator vit = varNodes->begin(), vend = varNodes->end(); vit != vend; ++vit) {
			SCC.insert(vit->second);
	}

	for (VarNodes::iterator vit = varNodes->begin(), vend = varNodes->end(); vit != vend; ++vit) {
			Value *V = const_cast<Value*>(vit->first);
			components[V] = SCC;
	}

	this->worklist.push_back(const_cast<Value*>(varNodes->begin()->first));
}
else {
	

	// Copy structures
	this->variables = varNodes;
	this->index = 0;
	
	// Iterate over all varnodes of the constraint graph
	for (VarNodes::iterator vit = varNodes->begin(), vend = varNodes->end(); vit != vend; ++vit) {
		// Initialize DFS control variable for each Value in the graph
		Value *V = const_cast<Value*>(vit->first);
		dfs[V] = -1;
	}
	
	// Self-explanatory
	addControlDependenceEdges(symbMap, useMap, varNodes);
	
	// Iterate again over all varnodes of the constraint graph
	for (VarNodes::iterator vit = varNodes->begin(), vend = varNodes->end(); vit != vend; ++vit) {
		Value *V = const_cast<Value*>(vit->first);
		
		// If the Value has not been visited yet, call visit for him
		if (dfs[V] < 0) {
			std::stack<Value*> pilha;
			if (!pilha.empty()) {
				errs() << "Erro na pilha\n";
			}
			
			visit(V, pilha, useMap);
		}
	}
	
	// Self-explanatory
	delControlDependenceEdges(useMap);
}
}
