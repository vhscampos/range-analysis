/*
 * Range.h
 *
 * Copyright (C) 2011-2012  Victor Hugo Sperle Campos
 * Copyright (C) 2013-2014  Raphael Ernani Rodrigues
 *
 */

#ifndef RANGE_H_
#define RANGE_H_

#include "llvm/Pass.h"
#include "llvm/ADT/APInt.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/raw_ostream.h"

#include <string>

//****************************************************************************//
using namespace llvm;

/// In our range analysis pass we have to perform operations on ranges all the
/// time. LLVM has a class to perform operations on ranges: the class
/// Range. However, the class Range doesn't serve very well
/// for our purposes because we need to perform operations with big numbers
/// (MIN_INT, MAX_INT) a lot of times, without allow these numbers wrap around.
/// And the class Range allows that. So, I'm writing this class to
/// perform operations on ranges, considering these big numbers and without
/// allow them wrap around.
/// The interface of this class is very similar to LLVM's ConstantRange class.
/// TODO: probably, a better idea would be perform our range analysis
/// considering the ranges symbolically, letting them wrap around,
/// as ConstantRange considers, but it would require big changes
/// in our algorithm.


// The number of bits needed to store the largest variable of the function (APInt).
extern unsigned MAX_BIT_INT;

// The min and max integer values for a given bit width.
extern APInt Min;
extern APInt Max;
extern APInt Zero;


enum RangeType {Unknown, Regular, Empty};

class Range {
private:
	APInt l;	// The lower bound of the range.
	APInt u;	// The upper bound of the range.
	RangeType type;

public:
	Range();
	Range(const Range &Other);
	Range(APInt lb, APInt ub, RangeType type = Regular);
	Range(std::string RangeString);
	~Range();
	APInt getLower() const {return l;}
	APInt getUpper() const {return u;}
	void setLower(const APInt& newl) {this->l = newl;}
	void setUpper(const APInt& newu) {this->u = newu;}
	bool isUnknown() const {return type == Unknown;}
	void setUnknown() {type = Unknown;}
	bool isRegular() const {return type == Regular;}
	void setRegular() {type = Regular;}
	bool isEmpty() const {return type == Empty;}
	void setEmpty() {type = Empty;}
	bool isMaxRange() const;
	void print(raw_ostream& OS) const;
	Range add(const Range& other);
	Range sub(const Range& other);
	Range mul(const Range& other);
	Range udiv(const Range& other);
	Range sdiv(const Range& other);
	Range urem(const Range& other);
	Range srem(const Range& other);
	Range shl(const Range& other);
	Range lshr(const Range& other);
	Range ashr(const Range& other);
	Range And(const Range& other);
	Range Or(const Range& other);
	Range Xor(const Range& other);
	Range truncate(unsigned bitwidth) const;
	Range sextOrTrunc(unsigned bitwidth) const;
	Range zextOrTrunc(unsigned bitwidth) const;
	Range intersectWith(const Range& other) const;
	Range unionWith(const Range& other) const;
	bool operator==(const Range& other) const;
	bool operator!=(const Range& other) const;
};

#endif /* RANGE_H_ */
