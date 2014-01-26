//===------------------------------- Range.cpp ----------------------------===//
//===----------------------------------------------------------------------===//

#include "Range.h"

#include "llvm/Pass.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ConstantRange.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Process.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TimeValue.h"

#include "ginac/ginac.h"

#include <algorithm>
#include <deque>
#include <set>
#include <sstream>
#include <stack>

/* ************************************************************************** */
/* ************************************************************************** */

namespace llvm {

raw_ostream& operator<<(raw_ostream& OS, const Range &R) {
  OS << "[" << R.getLower() << ", " << R.getUpper() << "]";
  return OS;
}

} // end namespace llvm


using namespace llvm;
using std::make_pair;
using std::pair;
using std::stringstream;

static cl::opt<bool>
  ExprDebug("range-debug",
          cl::desc("Debug operations on symbolic ranges"),
          cl::Hidden, cl::init(false));

#define RANGE_DEBUG(X) { if (ExprDebug) { X; } }

/*********
 * Range *
 *********/
Range::Range() {
}

Range::Range(Expr Both)
  : Lower_(Both), Upper_(Both) {
}

Range::Range(Expr Lower, Expr Upper)
  : Lower_(Lower), Upper_(Upper) {
}

Range::~Range() {
}

Range Range::meet(const Range& Other) const {
  // errs() << "MEET: " << Other << ", " << *this << "\n";
  return Range(Other.getLower().min(getLower()),
               Other.getUpper().max(getUpper()));
}

Range Range::widen(const Range& Other) const {
  RANGE_DEBUG(dbgs() << "widen() :: " << *this << " :: " << Other << "\n");
  if (Other.getLower() == Expr::GetBottomValue() ||
      Other.getUpper() == Expr::GetBottomValue()) {
    if (Other.getLower() != Other.getUpper())
      dbgs() << "widen() :: " << *this << " :: " << Other << "\n";
    assert(Other.getLower() == Other.getUpper() && "Inconsistent state");
    RANGE_DEBUG(dbgs() << "widen() = " << *this << "\n");
    return *this;
  }

  if (getLower() == Expr::GetBottomValue() ||
      getUpper() == Expr::GetBottomValue()) {
    assert(getLower() == getUpper() && "Inconsistent state");
    RANGE_DEBUG(dbgs() << "widen() = " << Other << "\n");
    return Other;
  }

  Expr Lower = Other.getLower() == getLower() ? getLower()
                                              : Expr::GetMinusInfValue();
  Expr Upper = Other.getUpper() == getUpper() ? getUpper()
                                              : Expr::GetPlusInfValue();
  RANGE_DEBUG(dbgs() << "widen() = " << Range(Lower, Upper) << "\n");
  return Range(Lower, Upper);
}

Range Range::operator+(const Range& Other) const {
  return Range(getLower() + Other.getLower(),
               getUpper() + Other.getUpper());
}

Range Range::operator-(const Range& Other) const {
  return Range(getLower() - Other.getUpper(),
               getUpper() - Other.getLower());
}

Range Range::operator*(const Range& Other) const {
  //errs() << "MUL: " << *this << " * " << Other << "\n";
  if (Other.getLower().isNegative() || getLower().isNegative()) {
    //errs() << "NEG\n";
    return Range(getUpper() * Other.getUpper(),
                 getLower() * Other.getUpper());
  } if (Other.getLower().isNonNegative() || getLower().isNonNegative()) {
    //errs() << "POS\n";
    return Range(getLower() * Other.getLower(),
                 getUpper() * Other.getUpper());
  }
  return GetInfRange();
}

Range Range::operator/(const Range& Other) const {
  if (Other.getLower().isStrictlyPositive() || getLower().isStrictlyPositive())
    return Range(getUpper() / Other.getUpper(),
                 getLower() / Other.getUpper());
  if (Other.getLower().isNegative() || getLower().isNegative())
    return Range(getLower() / Other.getLower(),
                 getUpper() / Other.getUpper());
  return GetInfRange();
}

Range Range::subs(std::vector<std::pair<Expr, Expr> > Subs) {
  return Range(getLower().subs(Subs), getUpper().subs(Subs));
}

Range Range::GetBottomRange() {
  static Range Bottom(Expr::GetBottomValue(), Expr::GetBottomValue());
  return Bottom;
}

Range Range::GetZeroRange() {
  static Range Zero(Expr::GetZeroValue(), Expr::GetZeroValue());
  return Zero;
}

Range Range::GetInfRange() {
  static Range Inf(Expr::GetMinusInfValue(), Expr::GetPlusInfValue());
  return Inf;
}

/* ************************************************************************** */
/* ************************************************************************** */

