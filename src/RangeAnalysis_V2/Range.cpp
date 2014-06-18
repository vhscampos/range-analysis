/*
 * Range.cpp
 *
 * Copyright (C) 2011-2012  Victor Hugo Sperle Campos
 * Copyright (C) 2013-2014  Raphael Ernani Rodrigues
 */

#include "Range.h"

// The number of bits needed to store the largest variable of the function (APInt).
unsigned MAX_BIT_INT = 1;

// The min and max integer values for a given bit width.
APInt Min = APInt::getSignedMinValue(MAX_BIT_INT);
APInt Max = APInt::getSignedMaxValue(MAX_BIT_INT);
APInt Zero(MAX_BIT_INT, 0, true);


// ========================================================================== //
// Range
// ========================================================================== //
Range::Range(): l(Min), u(Max), type(Regular) {
}

Range::Range(APInt lb, APInt ub, RangeType rType) :
		l(lb), u(ub), type(rType) {
  if (lb.sgt(ub))
    type = Empty;
}

int char2Int(char c){
	return c - '0';
}

APInt string2APInt(std::string IntString){
	bool minus = false;
	unsigned int firstPosition = 0;

	if (IntString.compare("-inf") == 0) return Min;
	if (IntString.compare("+inf") == 0) return Max;

	if (IntString.c_str()[0] == '-') {
		minus = true;
		firstPosition = 1;
	}

	uint64_t val = 0;
	uint64_t base = 1;

	for(unsigned int i = IntString.size() - 1; i >= firstPosition; i-- ){
		val += char2Int(IntString.c_str()[i]) * base;
		base *= 10;

		//Avoiding Overflow
		if (i==firstPosition) break;
	}

	if(minus) val = val * -1;

	return APInt(MAX_BIT_INT, val, true);

}

Range::Range(const Range &Other): l(Other.l), u(Other.u), type(Other.type){
}

Range::Range(std::string RangeString){

	//Remove square brackets
	std::string Contents = RangeString.substr(1,RangeString.size() -2);

	//Find comma
	std::size_t cSize = Contents.size();
	std::size_t commaPosition = Contents.find_first_of(",");

	//Split contents in two
	std::string lbStr = Contents.substr(0, commaPosition);
	std::string ubStr = Contents.substr(commaPosition+2, (cSize-commaPosition)-2);

	//Parse each of the bounds
	APInt lb =  string2APInt(lbStr);
	APInt ub =  string2APInt(ubStr);

	l = lb;
	u = ub;
	type = Regular;

}

Range::~Range() {
}


bool Range::isMaxRange() const {
	return this->getLower().eq(Min) && this->getUpper().eq(Max);
}

/// Add and Mul are commutatives. So, they are a little different
/// of the other operations.
Range Range::add(const Range& other) {
	const APInt &a = this->getLower();
	const APInt &b = this->getUpper();
	const APInt &c = other.getLower();
	const APInt &d = other.getUpper();
	APInt l = Min, u = Max;
	if (a.ne(Min) && c.ne(Min)) {
		l = a + c;
		if(a.isNegative() == c.isNegative() && a.isNegative() != l.isNegative())
			l = Min;
	}

	if (b.ne(Max) && d.ne(Max)) {
		u = b + d;
		if(b.isNegative() == d.isNegative() && b.isNegative() != u.isNegative())
			u = Max;
	}

	return Range(l, u);
}

/// [a, b] − [c, d] =
/// [min (a − c, a − d, b − c, b − d),
/// max (a − c, a − d, b − c, b − d)] = [a − d, b − c]
/// The other operations are just like this operation.
Range Range::sub(const Range& other) {
	const APInt &a = this->getLower();
	const APInt &b = this->getUpper();
	const APInt &c = other.getLower();
	const APInt &d = other.getUpper();
	APInt l, u;

	//a-d
	if (a.eq(Min) || d.eq(Max))
		l = Min;
	else
		//FIXME: handle overflow when a-d < Min
		l = a - d;

	//b-c
	if (b.eq(Max) || c.eq(Min))
		u = Max;
	else
		//FIXME: handle overflow when b-c > Max
		u = b - c;

	return Range(l, u);
}

#define MUL_HELPER(x,y) x.eq(Max) ? \
							(y.slt(Zero) ? Min : (y.eq(Zero) ? Zero : Max)) \
							: (y.eq(Max) ? \
								(x.slt(Zero) ? Min :(x.eq(Zero) ? Zero : Max)) \
								:(x.eq(Min) ? \
									(y.slt(Zero) ? Max : (y.eq(Zero) ? Zero : Min)) \
									: (y.eq(Min) ? \
										(x.slt(Zero) ? Max : (x.eq(Zero) ? Zero : Min)) \
										:(x*y))))

#define MUL_OV(x,y,xy) (x.isStrictlyPositive() == y.isStrictlyPositive() ? \
							(xy.isNegative() ? Max : xy) \
							: (xy.isStrictlyPositive() ? Min : xy))

/// Add and Mul are commutatives. So, they are a little different
/// of the other operations.
// [a, b] * [c, d] = [Min(a*c, a*d, b*c, b*d), Max(a*c, a*d, b*c, b*d)]
Range Range::mul(const Range& other) {
	if (this->isMaxRange() || other.isMaxRange()) {
		return Range(Min, Max);
	}

	const APInt &a = this->getLower();
	const APInt &b = this->getUpper();
	const APInt &c = other.getLower();
	const APInt &d = other.getUpper();

	APInt candidates[4];
	candidates[0] = MUL_OV(a, c, MUL_HELPER(a,c));
	candidates[1] = MUL_OV(a, d, MUL_HELPER(a,d));
	candidates[2] = MUL_OV(b, c, MUL_HELPER(b,c));
	candidates[3] = MUL_OV(b, d, MUL_HELPER(b,d));

	//Lower bound is the min value from the vector, while upper bound is the max value
	APInt *min = &candidates[0];
	APInt *max = &candidates[0];

	for (unsigned i = 1; i < 4; ++i) {
		if (candidates[i].sgt(*max))
			max = &candidates[i];
		else if (candidates[i].slt(*min))
			min = &candidates[i];
	}

	return Range(*min, *max);
}

#define DIV_HELPER(OP,x,y) 	x.eq(Max) ? \
						(y.slt(Zero) ? Min : (y.eq(Zero) ? Zero : Max)) \
						: (y.eq(Max) ? \
							(x.slt(Zero) ? Min :(x.eq(Zero) ? Zero : Max)) \
							:(x.eq(Min) ? \
								(y.slt(Zero) ? Max : (y.eq(Zero) ? Zero : Min)) \
								: (y.eq(Min) ? \
									(x.slt(Zero) ? Max : (x.eq(Zero) ? Zero : Min)) \
									:(x.OP(y)))))

Range Range::udiv(const Range& other) {
//	const APInt &a = this->getLower();
//	const APInt &b = this->getUpper();
//	const APInt &c = other.getLower();
//	const APInt &d = other.getUpper();

//	// Deal with division by 0 exception
//	if ((c.eq(Zero) || d.eq(Zero))) {
//		return Range(Min, Max);
//	}

//	APInt candidates[4];
//
//	// value[1]: lb(c) / leastpositive(d)
//	candidates[0] = DIV_HELPER(udiv, a, c);
//	candidates[1] = DIV_HELPER(udiv, a, d);
//	candidates[2] = DIV_HELPER(udiv, b, c);
//	candidates[3] = DIV_HELPER(udiv, b, d);

//	//Lower bound is the min value from the vector, while upper bound is the max value
//	APInt *min = &candidates[0];
//	APInt *max = &candidates[0];

//	for (unsigned i = 1; i < 4; ++i) {
//		if (candidates[i].sgt(*max))
//			max = &candidates[i];
//		else if (candidates[i].slt(*min))
//			min = &candidates[i];
//	}

//	return Range(*min, *max);


// This code has been copied from ::udiv() of ConstantRange class,
// and adapted to our purposes

	if (isEmpty() || other.isEmpty() || other.getUpper() == 0)
	    return Range(Min, Max);
	if (other.isMaxRange())
		return Range(Min, Max);

	APInt Lower = getLower().udiv(other.getUpper());

	APInt other_umin = other.getLower();

	if (other_umin == 0) {
      other_umin = APInt(MAX_BIT_INT, 1);
	}

	APInt Upper = getUpper().udiv(other_umin);

	// If the LHS is Full and the RHS is a wrapped interval containing 1 then
	// this could occur.
	if (Lower == Upper)
		return Range(Min, Max);

	return Range(Lower, Upper);
}

Range Range::sdiv(const Range& other) {
//	const APInt &a = this->getLower();
//	const APInt &b = this->getUpper();
//	const APInt &c = other.getLower();
//	const APInt &d = other.getUpper();

//	// Deal with division by 0 exception
//	if ((c.eq(Zero) || d.eq(Zero))) {
//		return Range(Min, Max);
//	}

//	APInt candidates[4];
//	candidates[0] = DIV_HELPER(sdiv, a, c);
//	candidates[1] = DIV_HELPER(sdiv, a, d);
//	candidates[2] = DIV_HELPER(sdiv, b, c);
//	candidates[3] = DIV_HELPER(sdiv, b, d);

//	//Lower bound is the min value from the vector, while upper bound is the max value
//	APInt *min = &candidates[0];
//	APInt *max = &candidates[0];

//	for (unsigned i = 1; i < 4; ++i) {
//		if (candidates[i].sgt(*max))
//			max = &candidates[i];
//		else if (candidates[i].slt(*min))
//			min = &candidates[i];
//	}

//	return Range(*min, *max);


	if (isEmpty() || other.isEmpty() || other.getUpper() == 0)
	    return Range(Min, Max);
	if (other.isMaxRange())
		return Range(Min, Max);

	APInt Lower = getLower().sdiv(other.getUpper());

	APInt other_smin = other.getLower();

	if (other_smin == 0) {
      other_smin = APInt(MAX_BIT_INT, 1);
	}

	APInt Upper = getUpper().sdiv(other_smin);

	// If the LHS is Full and the RHS is a wrapped interval containing 1 then
	// this could occur.
	if (Lower == Upper)
		return Range(Min, Max);

	return Range(Lower, Upper);
}

Range Range::urem(const Range& other) {
	const APInt &a = this->getLower();
	const APInt &b = this->getUpper();
	const APInt &c = other.getLower();
	const APInt &d = other.getUpper();

	// Deal with mod 0 exception
	if (c.eq(Zero) || d.eq(Zero)) {
		return Range(Min, Max);
	}

	APInt candidates[4];
	candidates[0] = Min;
	candidates[1] = Min;
	candidates[2] = Max;
	candidates[3] = Max;

	if (a.ne(Min) && c.ne(Min)) {
		candidates[0] = a.urem(c); // lower lower
	}

	if (a.ne(Min) && d.ne(Max)) {
		candidates[1] = a.urem(d); // lower upper
	}

	if (b.ne(Max) && c.ne(Min)) {
		candidates[2] = b.urem(c); // upper lower
	}

	if (b.ne(Max) && d.ne(Max)) {
		candidates[3] = b.urem(d); // upper upper
	}

	//Lower bound is the min value from the vector, while upper bound is the max value
	APInt *min = &candidates[0];
	APInt *max = &candidates[0];

	for (unsigned i = 1; i < 4; ++i) {
		if (candidates[i].sgt(*max))
			max = &candidates[i];
		else if (candidates[i].slt(*min))
			min = &candidates[i];
	}

	return Range(*min, *max);
}

Range Range::srem(const Range& other) {
	if(other == Range(Zero,Zero) || other == Range(Min, Max, Empty))
		return Range(Min, Max, Empty);

	const APInt &a = this->getLower();
	const APInt &b = this->getUpper();
	const APInt &c = other.getLower();
	const APInt &d = other.getUpper();

	// Deal with mod 0 exception
	if((c.slt(Zero) && d.sgt(Zero)) || c.eq(Zero) || d.eq(Zero))
		return Range(Min, Max);

	APInt candidates[4];
	candidates[0] = Min;
	candidates[1] = Min;
	candidates[2] = Max;
	candidates[3] = Max;

	if (a.ne(Min) && c.ne(Min)) {
		candidates[0] = a.srem(c); // lower lower
	}

	if (a.ne(Min) && d.ne(Max)) {
		candidates[1] = a.srem(d); // lower upper
	}

	if (b.ne(Max) && c.ne(Min)) {
		candidates[2] = b.srem(c); // upper lower
	}

	if (b.ne(Max) && d.ne(Max)) {
		candidates[3] = b.srem(d); // upper upper
	}

	//Lower bound is the min value from the vector, while upper bound is the max value
	APInt *min = &candidates[0];
	APInt *max = &candidates[0];

	for (unsigned i = 1; i < 4; ++i) {
		if (candidates[i].sgt(*max))
			max = &candidates[i];
		else if (candidates[i].slt(*min))
			min = &candidates[i];
	}

	return Range(*min, *max);
}

Range Range::shl(const Range& other) {
	const APInt &a = this->getLower();
	const APInt &b = this->getUpper();
	const APInt &c = other.getLower();
	const APInt &d = other.getUpper();

	APInt candidates[4];
	candidates[0] = Min;
	candidates[1] = Min;
	candidates[2] = Max;
	candidates[3] = Max;

	if (a.ne(Min) && c.ne(Min)) {
		candidates[0] = a.shl(c); // lower lower
	}

	if (a.ne(Min) && d.ne(Max)) {
		candidates[1] = a.shl(d); // lower upper
	}

	if (b.ne(Max) && c.ne(Min)) {
		candidates[2] = b.shl(c); // upper lower
	}

	if (b.ne(Max) && d.ne(Max)) {
		candidates[3] = b.shl(d); // upper upper
	}

	//Lower bound is the min value from the vector, while upper bound is the max value
	APInt *min = &candidates[0];
	APInt *max = &candidates[0];

	for (unsigned i = 1; i < 4; ++i) {
		if (candidates[i].sgt(*max))
			max = &candidates[i];
		else if (candidates[i].slt(*min))
			min = &candidates[i];
	}

	return Range(*min, *max);
}

Range Range::lshr(const Range& other) {
	const APInt &a = this->getLower();
	const APInt &b = this->getUpper();
	const APInt &c = other.getLower();
	const APInt &d = other.getUpper();

	// If any of the bounds is negative, result is [0, +inf] automatically
	if (a.isNegative() || b.isNegative()) {
		return Range(Zero, Max);
	}

	APInt candidates[4];
	candidates[0] = Min;
	candidates[1] = Min;
	candidates[2] = Max;
	candidates[3] = Max;

	if (a.ne(Min) && c.ne(Min)) {
		candidates[0] = a.lshr(c); // lower lower
	}

	if (a.ne(Min) && d.ne(Max)) {
		candidates[1] = a.lshr(d); // lower upper
	}

	if (b.ne(Max) && c.ne(Min)) {
		candidates[2] = b.lshr(c); // upper lower
	}

	if (b.ne(Max) && d.ne(Max)) {
		candidates[3] = b.lshr(d); // upper upper
	}

	//Lower bound is the min value from the vector, while upper bound is the max value
	APInt *min = &candidates[0];
	APInt *max = &candidates[0];

	for (unsigned i = 1; i < 4; ++i) {
		if (candidates[i].sgt(*max))
			max = &candidates[i];
		else if (candidates[i].slt(*min))
			min = &candidates[i];
	}

	return Range(*min, *max);
}

Range Range::ashr(const Range& other) {
	const APInt &a = this->getLower();
	const APInt &b = this->getUpper();
	const APInt &c = other.getLower();
	const APInt &d = other.getUpper();

	APInt candidates[4];
	candidates[0] = Min;
	candidates[1] = Min;
	candidates[2] = Max;
	candidates[3] = Max;

	if (a.ne(Min) && c.ne(Min)) {
		candidates[0] = a.ashr(c); // lower lower
	}

	if (a.ne(Min) && d.ne(Max)) {
		candidates[1] = a.ashr(d); // lower upper
	}

	if (b.ne(Max) && c.ne(Min)) {
		candidates[2] = b.ashr(c); // upper lower
	}

	if (b.ne(Max) && d.ne(Max)) {
		candidates[3] = b.ashr(d); // upper upper
	}

	//Lower bound is the min value from the vector, while upper bound is the max value
	APInt *min = &candidates[0];
	APInt *max = &candidates[0];

	for (unsigned i = 1; i < 4; ++i) {
		if (candidates[i].sgt(*max))
			max = &candidates[i];
		else if (candidates[i].slt(*min))
			min = &candidates[i];
	}

	return Range(*min, *max);
}

int64_t minAND(int64_t a, int64_t b, int64_t c, int64_t d)
{
	int64_t m, temp;

	m = 0x80000000;
	while (m != 0) {
		if (~a & ~c & m) {
			temp = (a | m) & -m;
			if (temp <= b) {
				a = temp;
				break;
			}
			temp = (c | m) & -m;
			if (temp <= d) {
				c = temp;
				break;
			}
		}
		m = m >> 1;
	}
	return a & c;
}

int64_t maxAND(int64_t a, int64_t b, int64_t c, int64_t d)
{
	int64_t m, temp;

	m = 0x80000000;
	while (m != 0) {
		if (b & ~d & m) {
			temp = (b & ~m) | (m - 1);
			if (temp >= a) {
				b = temp;
				break;
			}
		}
		else if (~b & d & m) {
			temp = (d & ~m) | (m - 1);
			if (temp >= c) {
				d = temp;
				break;
			}
		}
		m = m >> 1;
	}
	return b & d;
}

/*
 *  This and operation was took from the original ConstantRange implementation.
 *  It does not provide a tight result.
 *  TODO: Implement Hacker's Delight algorithm
 */
Range Range::And(const Range& other) {
	if (isEmpty() || other.isEmpty())
		return Range(Min, Max, Empty);

	APInt umin = APIntOps::umin(other.getUpper(), getUpper());
	if (umin.isAllOnesValue())
		return Range(Min, Max);
	return Range(APInt::getNullValue(MAX_BIT_INT), umin);
}

int64_t minOR(int64_t a, int64_t b, int64_t c, int64_t d)
{
	int64_t m, temp;

	m = 0x80000000;
	while (m != 0) {
		if (~a & c & m) {
			temp = (a | m) & -m;
			if (temp <= b) {
				a = temp;
				break;
			}
		}
		else if (a & ~c & m) {
			temp = (c | m) & -m;
			if (temp <= d) {
				c = temp;
				break;
			}
		}
		m = m >> 1;
	}
	return a | c;
}

int64_t maxOR(int64_t a, int64_t b, int64_t c, int64_t d)
{
	int64_t m, temp;

	m = 0x80000000;
	while (m != 0) {
		if (b & d & m) {
			temp = (b - m) | (m - 1);
			if (temp >= a) {
				b = temp;
				break;
			}
			temp = (d - m) | (m - 1);
			if (temp >= c) {
				d = temp;
				break;
			}
		}
		m = m >> 1;
	}
	return b | d;
}


/*
 * 	This or operation is coded following Hacker's Delight algorithm.
 * 	According to the author, it provides tight results.
 */
Range Range::Or(const Range& other) {
	const APInt &a = this->getLower();
	const APInt &b = this->getUpper();
	const APInt &c = other.getLower();
	const APInt &d = other.getUpper();

	if (this->isUnknown() || other.isUnknown()) {
		return Range(Min, Max, Unknown);
	}

	unsigned char switchval = 0;
	switchval += (a.isNonNegative() ? 1 : 0);
	switchval <<= 1;
	switchval += (b.isNonNegative() ? 1 : 0);
	switchval <<= 1;
	switchval += (c.isNonNegative() ? 1 : 0);
	switchval <<= 1;
	switchval += (d.isNonNegative() ? 1 : 0);

	APInt l = Min, u = Max;

	switch (switchval) {
		case 0:
			l = minOR(a.getSExtValue(), b.getSExtValue(), c.getSExtValue(), d.getSExtValue());
			u = maxOR(a.getSExtValue(), b.getSExtValue(), c.getSExtValue(), d.getSExtValue());
			break;
		case 1:
			l = a;
			u = -1;
			break;
		case 3:
			l = minOR(a.getSExtValue(), b.getSExtValue(), c.getSExtValue(), d.getSExtValue());
			u = maxOR(a.getSExtValue(), b.getSExtValue(), c.getSExtValue(), d.getSExtValue());
			break;
		case 4:
			l = c;
			u = -1;
			break;
		case 5:
			l = (a.slt(c) ? a : c);
			u = maxOR(0, b.getSExtValue(), 0, d.getSExtValue());
			break;
		case 7:
			l = minOR(a.getSExtValue(), 0xFFFFFFFF, c.getSExtValue(), d.getSExtValue());
			u = minOR(0, b.getSExtValue(), c.getSExtValue(), d.getSExtValue());
			break;
		case 12:
			l = minOR(a.getSExtValue(), b.getSExtValue(), c.getSExtValue(), d.getSExtValue());
			u = maxOR(a.getSExtValue(), b.getSExtValue(), c.getSExtValue(), d.getSExtValue());
			break;
		case 13:
			l = minOR(a.getSExtValue(), b.getSExtValue(), c.getSExtValue(), 0xFFFFFFFF);
			u = maxOR(a.getSExtValue(), b.getSExtValue(), 0, d.getSExtValue());
			break;
		case 15:
			l = minOR(a.getSExtValue(), b.getSExtValue(), c.getSExtValue(), d.getSExtValue());
			u = maxOR(a.getSExtValue(), b.getSExtValue(), c.getSExtValue(), d.getSExtValue());
			break;
	}

	return Range(l, u);
}

int64_t minXOR(int64_t a, int64_t b, int64_t c, int64_t d)
{
	return minAND(a, b, ~d, ~c) | minAND(~b, ~a, c, d);
}

int64_t maxXOR(int64_t a, int64_t b, int64_t c, int64_t d)
{
	return maxOR(0, maxAND(a, b, ~d, ~c), 0, maxAND(~b, ~a, c, d));
}

/*
 * 	We don't have a xor implementation yet.
 * 	To be in safe side, we just give maxrange as result.
 */
Range Range::Xor(const Range& other) {
	return Range(Min, Max);
}

// Truncate
//		- if the source range is entirely inside max bit range, he is the result
//      - else, the result is the max bit range
Range Range::truncate(unsigned bitwidth) const {
	APInt maxupper = APInt::getSignedMaxValue(bitwidth);
	APInt maxlower = APInt::getSignedMinValue(bitwidth);

	if (bitwidth < MAX_BIT_INT) {
		maxupper = maxupper.sext(MAX_BIT_INT);
		maxlower = maxlower.sext(MAX_BIT_INT);
	}

	// Check if source range is contained by max bit range
	if (this->getLower().sge(maxlower) && this->getUpper().sle(maxupper)) {
		return *this;
	} else {
		return Range(maxlower, maxupper);
	}
}

Range Range::sextOrTrunc(unsigned bitwidth) const {
	return truncate(bitwidth);
}

Range Range::zextOrTrunc(unsigned bitwidth) const {
	APInt maxupper = APInt::getSignedMaxValue(bitwidth);
	APInt maxlower = APInt::getSignedMinValue(bitwidth);

	if (bitwidth < MAX_BIT_INT) {
		maxupper = maxupper.sext(MAX_BIT_INT);
		maxlower = maxlower.sext(MAX_BIT_INT);
	}

	// Check if source range is contained by max bit range
	if (this->getLower().sge(maxlower) && this->getUpper().sle(maxupper)) {
		return *this;
	} else {
		return Range(maxlower, maxupper);
	}
}

Range Range::intersectWith(const Range& other) const {
	if (this->isEmpty() || other.isEmpty())
		return Range(Min, Max, Empty);

	if (this->isUnknown()) {
		return other;
	}

	if (other.isUnknown()) {
		return *this;
	}

	APInt l = getLower().sgt(other.getLower()) ? getLower() : other.getLower();
	APInt u = getUpper().slt(other.getUpper()) ? getUpper() : other.getUpper();
	return Range(l, u);
}

Range Range::unionWith(const Range& other) const {
	if (this->isEmpty()) {
		return other;
	}

	if (other.isEmpty()) {
		return *this;
	}

	if (this->isUnknown()) {
		return other;
	}

	if (other.isUnknown()) {
		return *this;
	}

	APInt l = getLower().slt(other.getLower()) ? getLower() : other.getLower();
	APInt u = getUpper().sgt(other.getUpper()) ? getUpper() : other.getUpper();
	return Range(l, u);
}

bool Range::operator==(const Range& other) const {
	return this->type == other.type && getLower().eq(other.getLower())
			&& getUpper().eq(other.getUpper());
}

bool Range::operator!=(const Range& other) const {
	return this->type != other.type || getLower().ne(other.getLower())
			|| getUpper().ne(other.getUpper());
}

void Range::print(raw_ostream& OS) const {
	if (this->isUnknown()) {
		OS << "Unknown";
		return;
	}

	if (this->isEmpty()) {
		OS << "Empty";
		return;
	}

	if (getLower().eq(Min)) {
		OS << "[-inf, ";
	} else {
		OS << "[" << getLower() << ", ";
	}

	if (getUpper().eq(Max)) {
		OS << "+inf]";
	} else {
		OS << getUpper() << "]";
	}
}

raw_ostream& operator<<(raw_ostream& OS, const Range& R) {
	R.print(OS);
	return OS;
}
