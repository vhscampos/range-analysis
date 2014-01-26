//===------------------------------- Expr.cpp -----------------------------===//
//===----------------------------------------------------------------------===//

#include "Expr.h"
#include "Range.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include "ginac/ginac.h"

#include <algorithm>
#include <map>
#include <queue>
#include <set>

/* ************************************************************************** */
/* ************************************************************************** */

namespace GiNaC {

struct less {
  bool operator()(GiNaC::ex const& l, GiNaC::ex const& r) const {
    return l.compare(r) < 0;
  }
};

} // end namespace GiNaC

/* ************************************************************************** */
/* ************************************************************************** */

using namespace llvm;
using std::make_pair;
using std::map;
using std::max;
using std::pair;
using std::queue;
using std::set;
using std::string;
using std::stringstream;

static cl::opt<bool>
  ExprDebug("expr-debug",
          cl::desc("Debug operations on symbolic expressions"),
          cl::Hidden, cl::init(false));

char TestExpr::ID = 0;
static RegisterPass<TestExpr> X("expr-test", "Test expression handling");

#define EXPR_DEBUG(X) { if (ExprDebug) { X; } }

/* ************************************************************************** */
/* ************************************************************************** */

// Round
static long int Round(double D) {
  return (D > 0.0) ? (D + 0.5) : (D - 0.5); 
}

// GetSymbols
set<GiNaC::symbol, GiNaC::less> GetSymbols(GiNaC::ex Ex) {
  set<GiNaC::symbol, GiNaC::less> Syms;
  for (auto EI = Ex.preorder_begin(), E = Ex.preorder_end(); EI != E; ++EI)
    if (GiNaC::is_a<GiNaC::symbol>(*EI))
      Syms.insert(GiNaC::ex_to<GiNaC::symbol>(*EI));

  return Syms;
}

// GetCoeffs
vector<GiNaC::ex> GetCoeffs(GiNaC::ex Ex, GiNaC::symbol Sym) {
  assert(Ex.is_polynomial(Sym));
  vector<GiNaC::ex> Coeffs;
  unsigned Degree = Ex.degree(Sym);
  for (unsigned Idx = 0; Idx <= Degree; ++Idx)
    Coeffs.push_back(Ex.coeff(Sym, Idx));
  return Coeffs;
}

// Solve
vector<double> Solve(GiNaC::ex Ex, GiNaC::symbol Sym) {
  vector<double> Roots;

  unsigned Degree = Ex.degree(Sym);
  auto Coeffs = GetCoeffs(Ex, Sym);
  // Bhaskara.
  if (Degree == 2) {
    GiNaC::ex A = Coeffs[2];
    GiNaC::ex B = Coeffs[1];
    GiNaC::ex C = Coeffs[0];

    GiNaC::ex Delta = B*B - 4 * A * C;
    // Guaranteed real roots.
    if (GiNaC::is_a<GiNaC::numeric>(Delta) &&
        !GiNaC::ex_to<GiNaC::numeric>(Delta).is_negative()) {
      GiNaC::ex Delta = GiNaC::sqrt(B*B - 4 * A * C).evalf();
      GiNaC::ex One = ((-B) + Delta)/(2*A);
      GiNaC::ex Two = ((-B) - Delta)/(2*A);
      if (GiNaC::is_a<GiNaC::numeric>(One))
        Roots.push_back(GiNaC::ex_to<GiNaC::numeric>(One.evalf()).to_double());
      if (GiNaC::is_a<GiNaC::numeric>(Two))
        Roots.push_back(GiNaC::ex_to<GiNaC::numeric>(Two.evalf()).to_double());
    }
  }
  // Cardano.
  else if (Degree == 3) {
    GiNaC::ex A = Coeffs[3];
    GiNaC::ex B = Coeffs[2];
    GiNaC::ex C = Coeffs[1];
    GiNaC::ex D = Coeffs[1];

    GiNaC::ex Delta0 = B*B - 3 * A * C;
    GiNaC::ex Delta1 = 2 * B*B*B - 9 * A * B * C + 27 * A * A * D;
    GiNaC::ex CD = Delta1 + GiNaC::sqrt(Delta1 * Delta1 - 4 * GiNaC::pow(Delta0, 3));
    CD = CD/2;
    CD = GiNaC::pow(CD, GiNaC::numeric(1)/3);

    GiNaC::symbol U("u");
    GiNaC::ex Var = GiNaC::numeric(-1)/(3 * A) * (B + U * CD + Delta0/(U * CD));
    GiNaC::ex One   = Var.subs(U == 1);
    GiNaC::ex Two   = Var.subs(U == ((-1 + GiNaC::sqrt(GiNaC::numeric(-3)))/2));
    GiNaC::ex Three = Var.subs(U == ((-1 - GiNaC::sqrt(GiNaC::numeric(-3)))/2));
    if (GiNaC::is_a<GiNaC::numeric>(One))
      Roots.push_back(GiNaC::ex_to<GiNaC::numeric>(One.evalf()).to_double());
    if (GiNaC::is_a<GiNaC::numeric>(Two))
      Roots.push_back(GiNaC::ex_to<GiNaC::numeric>(Two.evalf()).to_double());
    if (GiNaC::is_a<GiNaC::numeric>(Three))
      Roots.push_back(GiNaC::ex_to<GiNaC::numeric>(Three.evalf()).to_double());
  }

  return Roots;
}

// NegativeOrdinate
// Returns the ranges for which the ordinate is negative.
vector<pair<GiNaC::ex, GiNaC::ex> >
    NegativeOrdinate(vector<double> Vec, GiNaC::symbol Sym, GiNaC::ex Ex) {
  vector<pair<GiNaC::ex, GiNaC::ex> > Negs;

  // No real roots - it's either all positive or all negative.
  if (Vec.empty()) {
    GiNaC::ex Subs = Ex.subs(Sym == 0);
    if (GiNaC::is_a<GiNaC::numeric>(Subs) &&
        GiNaC::ex_to<GiNaC::numeric>(Subs).is_negative())
        Negs.push_back(make_pair(GiNaC::inf(-1), GiNaC::inf(1)));
    return Negs;
  }

  sort(Vec.begin(), Vec.end());

  GiNaC::ex FirstSubs = Ex.subs(Sym == GiNaC::numeric(Round(Vec[0] - 1.0f)));
  // Negative at -inf to the first root minus one.
  if (GiNaC::is_a<GiNaC::numeric>(FirstSubs) &&
      GiNaC::ex_to<GiNaC::numeric>(FirstSubs).is_negative())
    Negs.push_back(make_pair(GiNaC::inf(-1), GiNaC::numeric(Vec[0] - 1.0f)));

  for (unsigned Idx = 1; Idx < Vec.size(); ++Idx) {
    long int E = Round(Vec[Idx]);
    // Check if it's negative to the right.
    GiNaC::ex Subs = Ex.subs(Sym == GiNaC::numeric(E + 1));
    if (GiNaC::is_a<GiNaC::numeric>(Subs))
      if (GiNaC::ex_to<GiNaC::numeric>(Subs).is_negative()) {
        // Last root, it's negative all the way to +inf.
        if (Idx == Vec.size() - 1)
          Negs.push_back(make_pair(GiNaC::numeric(E + 1), GiNaC::inf(1)));
        // Not the last root, it's negative from this root to the next.
        else
          Negs.push_back(make_pair(GiNaC::numeric(E + 1),
                                   GiNaC::numeric(Round(Vec[Idx + 1] - 1))));
      }
  }

  return Negs;
}

// ExIsLessThen
// Tells us if an expression is less then another assuming
// a symbol maps to a range in the map.
bool ExIsLessThan(GiNaC::ex LHS, GiNaC::ex RHS, map<Expr, Range> Assume = map<Expr, Range>()) {
  // Check if the substraction is a number. If so, LHS < RHS if LHS - RHS
  // is less than zero.
  GiNaC::ex Sub = (LHS - RHS).evalf();
  if (GiNaC::is_a<GiNaC::numeric>(Sub))
    return GiNaC::ex_to<GiNaC::numeric>(Sub).is_negative();

  /* auto Syms = GetSymbols(Sub);
  if (Syms.empty())
    return false;
  if (!Sub.is_polynomial(*Syms.begin()))
    return false;
  auto SubSolve = Solve(Sub, *Syms.begin());
  auto SubNegs  = NegativeOrdinate(SubSolve, *Syms.begin(), Sub); 

  if (SubNegs.size() == 0)
    return false;

  for (auto& P : SubNegs) {
    if (P.first == GiNaC::inf(-1) && P.second == GiNaC::inf(1))
      return true;
    // TODO:
    //if (P.first == inf(-1) &&
    //    GiNaC::ex_to<GiNaC::numeric>(P.second).to_int()
    //  return true;
  } */

  return false;
}

static bool ExIsGreaterThan(GiNaC::ex LHS, GiNaC::ex RHS) {
  assert(false);
}

/* ************************************************************************** */
/* ************************************************************************** */

/*********
 * Expr *
 *********/
Expr::Expr() {
}

Expr::Expr(int Int)
  : Expr_(Int) {
}

Expr::Expr(APInt Int)
  : Expr_((long int)Int.getSExtValue()) {
}

Expr::Expr(GiNaC::ex Expr)
  : Expr_(Expr) {
}

Expr::Expr(Twine Name)
  : Expr_(GiNaC::symbol(Name.str())) {
}

static DenseMap<std::pair<Value*, int>, GiNaC::ex> Exprs;
static std::map<std::string, Value*> RevExprs;

void Expr::init(Value* V, int level){
	//Level 0: stop recursion
	//Level 1: recursion just one time
	//Level 2: unlimited recursion

  assert(V && "Constructor expected non-null parameter");
  
  if (const ConstantInt *CI = dyn_cast<ConstantInt>(V)) {
    Expr_ = GiNaC::ex((long int)CI->getValue().getSExtValue());
    return;
  }

  if (Exprs.count(std::pair<Value*, int>(V,level))) {
    Expr_ = Exprs[std::pair<Value*, int>(V,level)];
    return;
  }

  //Level 0: stop recursion
  if (level == 0) {
	  Twine Name;
	  if (V->hasName()) {
		if (isa<Instruction>(V) || isa<Argument>(V))
		  Name = V->getName();
		else
		  Name = "__SRA_SYM_UNKNOWN_" + V->getName() + "__";
	  } else {
		Name = "__SRA_SYM_UNAMED__";
	  }

	  std::string NameStr = Name.str();
	  Expr_ = GiNaC::ex(GiNaC::symbol(NameStr));
	  Exprs[std::pair<Value*, int>(V,level)] = Expr_;

	  RevExprs[NameStr] = V;

	  return;
  }

  int old_level = level;
  level = (level == 1 ? 0 : 2);


  //Handle instructions

  if (const BinaryOperator* BI = dyn_cast<llvm::BinaryOperator>(V)){

	  switch (BI->getOpcode()){
	  case Instruction::Add:
	  {
		  Expr ADD_Op1(BI->getOperand(0), level);
		  Expr ADD_Op2(BI->getOperand(1), level);
		  Expr_ = (ADD_Op1.getExpr() + ADD_Op2.getExpr());
		  Exprs[std::pair<Value*, int>(V,old_level)] = Expr_;
		  return;
	  }
	  case Instruction::Sub:
	  {
		  Expr SUB_Op1(BI->getOperand(0), level);
		  Expr SUB_Op2(BI->getOperand(1), level);
		  Expr_ = (SUB_Op1.getExpr() + SUB_Op2.getExpr());
		  Exprs[std::pair<Value*, int>(V,old_level)] = Expr_;
		  return;
	  }
	  case Instruction::Mul:
	  {
		  Expr MUL_Op1(BI->getOperand(0), level);
		  Expr MUL_Op2(BI->getOperand(1), level);
		  Expr_ = (MUL_Op1.getExpr() * MUL_Op2.getExpr());
		  Exprs[std::pair<Value*, int>(V,old_level)] = Expr_;
		  return;
	  }
	  case Instruction::SDiv:
	  case Instruction::UDiv:
	  {
		  Expr DIV_Op1(BI->getOperand(0), level);
		  Expr DIV_Op2(BI->getOperand(1), level);
		  Expr_ = (DIV_Op1.getExpr() / DIV_Op2.getExpr());
		  Exprs[std::pair<Value*, int>(V,old_level)] = Expr_;
		  return;
	  }
	  default:
		  break;
	  }

  }

  if (const CastInst* CI = dyn_cast<CastInst>(V)){
	  Expr CAST_Op(CI->getOperand(0), old_level);
	  Expr_ = CAST_Op.getExpr();
	  Exprs[std::pair<Value*, int>(V,old_level)] = Expr_;
	  return;
  }


  //Unhandled instructions: treat them like level 0
  Expr tmp(V,0);
  Expr_ = tmp.getExpr();

}


Expr::Expr(Value *V) {
	init(V,0);
}

Expr::Expr(Value *V, int level) {
	init(V,level);
}

Expr Expr::subs(std::vector<std::pair<Expr, Expr> > Subs) { 
  GiNaC::ex Ex = Expr_;
  for (auto& E : Subs) {
    //dbgs() << "Replacing " << E.first.getExpr() << " with " << E.second.getExpr() << " in " << Ex;
    Ex = Ex.subs(E.first.getExpr() == E.second.getExpr());
    //dbgs() << " giving " << Ex << "\n";
  }
  return Expr(Ex);
}

bool Expr::lt(const Expr& Other, std::map<Expr, Range> Assume) const {
  //for (auto& P : Assume)
  //  assert(GiNaC::is_a<GiNaC::symbol>(P.first.getExpr()) &&
  //         "Assumptions must map symbols to range");
  return ExIsLessThan(Expr_, Other.getExpr()); 
}

bool Expr::le(const Expr& Other) const {
  return ExIsLessThan(Expr_, Other.getExpr() = 1);
}

bool Expr::gt(const Expr& Other) const {
  return ExIsGreaterThan(Expr_, Other.getExpr());
}

bool Expr::ge(const Expr& Other) const {
  return ExIsGreaterThan(Expr_, Other.getExpr() + 1);
}

bool Expr::eq(const Expr& Other) const {
  return Expr_.is_equal(Other.getExpr());
}

bool Expr::ne(const Expr& Other) const {
  return !Expr_.is_equal(Other.getExpr());
}

bool Expr::operator==(const Expr& Other) const {
  return eq(Other);
}

bool Expr::operator!=(const Expr& Other) const {
  return ne(Other);
}

Expr Expr::shr(const Expr& Other) const {
  assert(false);
}

Expr Expr::shl(const Expr& Other) const {
  assert(false);
}

Expr Expr::rem(const Expr& Other) const {
  assert(false);
}

Expr Expr::div(const Expr& Other) const {
  assert(false);
}

Expr Expr::min(const Expr& Other) const {
  if (Other == GetBottomValue())
    return *this;
  else if (*this == GetBottomValue())
    return Other;
  if (ExIsLessThan(Expr_, Other.getExpr()))
    return Expr_;
  else if (ExIsLessThan(Other.getExpr(), Expr_))
    return Other.getExpr();
  else if (GiNaC::is_a<GiNaC::numeric>(Expr_))
    return Expr_;
  else if (GiNaC::is_a<GiNaC::numeric>(Other.getExpr()))
    return Other.getExpr();
    
  GiNaC::ex Res = GiNaC::min(Expr_, Other.getExpr());
  EXPR_DEBUG(dbgs() << "min(): " << *this << " :: " << Other
                    << " = " << Res << "\n");
  return Expr(Res);
}

Expr Expr::max(const Expr& Other) const {
  if (Other == GetBottomValue())
    return *this;
  else if (*this == GetBottomValue())
    return Other;

  if (ExIsLessThan(Expr_, Other.getExpr()))
    return Expr_;
  else if (ExIsLessThan(Other.getExpr(), Expr_))
    return Expr_;
  else if (GiNaC::is_a<GiNaC::numeric>(Expr_))
    return Other.getExpr();
  else if (GiNaC::is_a<GiNaC::numeric>(Other.getExpr()))
    return Other.getExpr();

  return Expr(GiNaC::max(Expr_, Other.getExpr()));
}

bool Expr::isNegative() const {
  if (GiNaC::is_a<GiNaC::numeric>(Expr_))
    return GiNaC::ex_to<GiNaC::numeric>(Expr_).is_negative();
  return isMinusInf();
}

bool Expr::isNonNegative() const {
  if (GiNaC::is_a<GiNaC::numeric>(Expr_))
    return !GiNaC::ex_to<GiNaC::numeric>(Expr_).is_negative();
  return isPlusInf();
}

bool Expr::isNotPositive() const {
  if (GiNaC::is_a<GiNaC::numeric>(Expr_))
    return !GiNaC::ex_to<GiNaC::numeric>(Expr_).is_positive();
  if (isPlusInf())
    return false;
  if (isMinusInf())
    return true;
  return ExIsLessThan(Expr_, GiNaC::numeric(1));
}

bool Expr::isStrictlyPositive() const {
  if (GiNaC::is_a<GiNaC::numeric>(Expr_))
    return GiNaC::ex_to<GiNaC::numeric>(Expr_).is_positive();
  if (isPlusInf())
    return true;
  return ExIsLessThan(GiNaC::numeric(0), Expr_);
}

bool Expr::isPlusInf() const {
  if (GiNaC::is_a<GiNaC::function>(Expr_)) {
    GiNaC::function Fn = GiNaC::ex_to<GiNaC::function>(Expr_);
    return Fn.get_name() == "inf" &&
           GiNaC::ex_to<GiNaC::numeric>(Fn.op(0)).is_positive();
  }
  return false;
}

bool Expr::isMinusInf() const {
  if (GiNaC::is_a<GiNaC::function>(Expr_)) {
    GiNaC::function Fn = GiNaC::ex_to<GiNaC::function>(Expr_);
    return Fn.get_name() == "inf" &&
           GiNaC::ex_to<GiNaC::numeric>(Fn.op(0)).is_negative();
  }
  return false;
}

bool Expr::isNumber() const {
  return GiNaC::is_a<GiNaC::numeric>(Expr_);
}

Expr Expr::operator+(const Expr& Other) const {
  return Expr_ + Other.getExpr();
}

Expr Expr::operator+(unsigned Other) const {
  return Expr_ + Other;
}

Expr Expr::operator-(const Expr& Other) const {
  return Expr_ - Other.getExpr();
}

Expr Expr::operator-(unsigned Other) const {
  return Expr_ - Other;
}

Expr Expr::operator*(const Expr& Other) const {
  return Expr_ * Other.getExpr();
}

Expr Expr::operator*(unsigned Other) const {
  return Expr_ * Other;
}

Expr Expr::operator/(const Expr& Other) const {
  return Expr_/Other.getExpr();
}

Expr Expr::operator/(unsigned Other) const {
  return Expr_/Other;
}

Expr Expr::GetPlusInfValue() {
  static Expr PlusInf(GiNaC::inf(GiNaC::numeric(1)));
  return PlusInf;
}

Expr Expr::GetMinusInfValue() {
  static Expr MinusInf(GiNaC::inf(GiNaC::numeric(-1)));
  return MinusInf;
}

Expr Expr::GetZeroValue() {
  static Expr Zero(GiNaC::numeric(0));
  return Zero;
}

Expr Expr::GetBottomValue() {
  static Expr Bottom(GiNaC::symbol("BOTTOM"));
  return Bottom;
}

int Expr::getNumber() const {

	assert(isNumber() && "Expression is not a number!");

	return Expr_.integer_content().to_int();
}

Value* Expr::getUniqueValue() const {

	//Unique operand
	if (Expr_.nops() > 1) return NULL;

	std::ostringstream Str;
	Str << Expr_;

	if (RevExprs.count(Str.str())) {
		return RevExprs[Str.str()];
	}

	return NULL;

}

GiNaC::ex Expr::getExpr() const {
  return Expr_;
}

int Expr::getMaxDegree() const {
  int Max = 0;
  for (auto It = Expr_.preorder_begin(), E = Expr_.preorder_end();
       It != E; ++It) {
    GiNaC::ex Ex = *It;
    if (GiNaC::is_a<GiNaC::power>(Ex))
      Max = std::max(Max, GiNaC::ex_to<GiNaC::numeric>(Ex.op(1)).to_int());
    else if (GiNaC::is_a<GiNaC::symbol>(Ex))
      Max = std::max(Max, 1);
  }
  return Max;
}

string Expr::getStringRepr() const {
  std::ostringstream Str;
  Str << Expr_;
  return Str.str();
}

namespace llvm {

raw_ostream& operator<<(raw_ostream& OS, const GiNaC::ex &E) {
  std::ostringstream Str;
  Str << E;
  OS << Str.str();
  return OS;
}

} // end namespace llvm

raw_ostream& operator<<(raw_ostream& OS, const Expr &EI) {
  std::ostringstream Str;
  Str << EI.getExpr();
  OS << Str.str();
  return OS;
}

/* ************************************************************************** */
/* ************************************************************************** */

bool TestExpr::runOnModule(Module&) {
  Expr A("a");
  Expr B("b");

  assert(A + B == B + A);
  assert(A * B == B * A);
  assert(A - B != B - A);
  assert(A / B != B / A);

  //map<GiNaC::symbol, int> GetMaxExps(GiNaC::ex Ex); 
  GiNaC::symbol GA("a");
  GiNaC::symbol GB("b");
  GiNaC::ex EZ = 5 * GA * GA * GA + 4 * GB + 1;

  assert(EZ.is_polynomial(GA));
  assert(EZ.degree(GA) == 3);
  assert(EZ.coeff(GA, 3) == GiNaC::numeric(5));
  assert(EZ.coeff(GA, 2) == GiNaC::numeric(0));
  assert(EZ.coeff(GA, 1) == GiNaC::numeric(0));
  assert(EZ.coeff(GA, 0) == 4 * GB + 1);

  assert(EZ.is_polynomial(GB));
  assert(EZ.degree(GB) == 1);
  assert(EZ.coeff(GB, 1) == GiNaC::numeric(4));
  assert(EZ.coeff(GB, 0) == 5 * GA * GA * GA + 1);

  auto SEZ = GetSymbols(EZ);
  assert(SEZ.size() == 2);
  assert(SEZ.count(GA) && SEZ.count(GB));

  auto CEZGA = GetCoeffs(EZ, GA);
  assert(CEZGA.size() == 4);
  assert(CEZGA[3] == GiNaC::numeric(5));
  assert(CEZGA[2] == GiNaC::numeric(0));
  assert(CEZGA[1] == GiNaC::numeric(0));
  assert(CEZGA[0] == 4 * GB + 1);

  auto CEZGB = GetCoeffs(EZ, GB);
  assert(CEZGB.size() == 2);
  assert(CEZGB[1] == GiNaC::numeric(4));
  assert(CEZGB[0] == 5 * GA * GA * GA + 1);

  auto EF = GA * GA - 1;
  auto EFSolve = Solve(EF, GA);
  assert(EFSolve.size() == 2);
  assert(Round(EFSolve[0]) == -1 || Round(EFSolve[1]) == -1);
  assert(Round(EFSolve[0]) == 1  || Round(EFSolve[1]) == 1);
  auto EFNegs = NegativeOrdinate(EFSolve, GA, EF); 
  assert(EFNegs.empty());

  EF = GA * GA - 2 * GA + 1;
  EFSolve = Solve(EF, GA);
  assert(EFSolve.size() == 2);
  assert(Round(EFSolve[0]) == 1 && Round(EFSolve[1]) == 1);
  EFNegs = NegativeOrdinate(EFSolve, GA, EF); 
  assert(EFNegs.empty());

  EF = (-1) * GA * GA + GA - 1;
  EFSolve = Solve(EF, GA);
  assert(EFSolve.size() == 0);
  EFNegs = NegativeOrdinate(EFSolve, GA, EF); 
  assert(EFNegs.size() == 1);
  assert(EFNegs[0].first.is_equal(GiNaC::inf(-1)));
  assert(EFNegs[0].second.is_equal(GiNaC::inf(1)));

  return false;
}

