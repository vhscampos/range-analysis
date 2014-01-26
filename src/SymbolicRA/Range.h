//===----------------------------- Range.cpp ------------------------------===//
//===----------------------------------------------------------------------===//

#ifndef _RANGE_H_
#define _RANGE_H_

#include "llvm/Pass.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/ConstantRange.h"
#include "llvm/Support/Process.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TimeValue.h"

#include "ginac/ginac.h"

#include <algorithm>
#include <deque>
#include <set>
#include <sstream>
#include <stack>

#include "Expr.h"

/* ************************************************************************** */
/* ************************************************************************** */

namespace llvm {

class Range {
public:
  Range();
  Range(Expr Both);
  Range(Expr Lower, Expr Upper);
	~Range();

  Range subs(std::vector<std::pair<Expr, Expr> > Subs); 

	Expr getLower() const { return Lower_; }
	Expr getUpper() const { return Upper_; }

	void setLower(const Expr& Lower) { Lower_ = Lower; }
	void setUpper(const Expr& Upper) { Upper_ = Upper; }

  bool lt(Range Other) { return Upper_.lt(Other.getLower()); }
  bool le(Range Other) { return Upper_.le(Other.getLower()); }

  // Basic arithmetic operations.
	Range add(const Range& Other);

  Range meet(const Range& Other) const;
  Range widen(const Range& Other) const;
  Range intersection(const Range& Other) const;

  Range operator+(const Range& Other) const;
  Range operator-(const Range& Other) const;
  Range operator*(const Range& Other) const;
  Range operator/(const Range& Other) const;

	bool operator==(const Range& Other) const;
	bool operator!=(const Range& Other) const;

	void print(raw_ostream& OS) const;

  static Range GetBottomRange(); 
  static Range GetZeroRange();
  static Range GetInfRange();

private:
	Expr Lower_; // The lower bound of the range.
	Expr Upper_; // The upper bound of the range.
};

raw_ostream& operator<<(raw_ostream& OS, const Range& R);

} // end namespace llvm

#endif

