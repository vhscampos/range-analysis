#ifndef _ENCAPSEXPR_H_
#define _ENCAPSEXPR_H_

#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"

#include "ginac/ginac.h"

#include <map>
#include <sstream>
#include <string>

namespace llvm {

class Range;
raw_ostream& operator<<(raw_ostream& OS, const GiNaC::ex& E);

} // end namespace llvm

using namespace llvm;
using std::map;
using std::pair;
using std::string;
using std::vector;

class Expr {
public:
  Expr();
  Expr(int Int);
  Expr(APInt Int);
  Expr(GiNaC::ex Expr);
  Expr(Twine Name);
  Expr(Value *V);
  Expr(Value* V, int level);

  Expr subs(vector<pair<Expr, Expr> > Subs); 

  bool lt        (const Expr& Other,
                  map<Expr, Range> Assume = map<Expr, Range>()) const;
  bool gt        (const Expr& Other) const;
  bool le        (const Expr& Other) const;
  bool ge        (const Expr& Other) const;
  bool eq        (const Expr& Other) const;
  bool ne        (const Expr& Other) const;
  bool operator==(const Expr& Other) const;
  bool operator!=(const Expr& Other) const;

  Expr shr      (const Expr& Other) const;
  Expr shl      (const Expr& Other) const;
  Expr rem      (const Expr& Other) const;
  Expr div      (const Expr& Other) const;
  Expr min      (const Expr& Other) const;
  Expr max      (const Expr& Other) const;
  Expr operator+(const Expr& Other) const;
  Expr operator+(unsigned Other)    const;
  Expr operator-(const Expr& Other) const;
  Expr operator-(unsigned Other)    const;
  Expr operator*(const Expr& Other) const;
  Expr operator*(unsigned Other)    const;
  Expr operator/(const Expr& Other) const;
  Expr operator/(unsigned Other)    const;

  int    getMaxDegree()  const;
  string getStringRepr() const;

  bool isNegative()         const;
  bool isNonNegative()      const;
  bool isNotPositive()      const;
  bool isStrictlyPositive() const;
  bool isPlusInf()          const;
  bool isMinusInf()         const;
  bool isNumber()           const;

  int getNumber()			const;
  Value* getUniqueValue()	const;

  static GiNaC::symbol& GetSymbol(const Value *V);

  static Expr           GetZeroValue();
  static Expr           GetPlusInfValue();
  static Expr           GetMinusInfValue();
  static Expr           GetBottomValue();

  static Expr Min(Expr L, Expr R);
  static Expr Max(Expr L, Expr R);

  friend raw_ostream& operator<<(raw_ostream& OS, const Expr& EI);
  friend bool ExIsLessThan(Expr LHS, Expr RHS, map<Expr, Range> Assume); 

protected:
  GiNaC::ex getExpr() const;

private:
  GiNaC::ex Expr_;
  void init(Value* V, int level);
};

class TestExpr : public ModulePass {
public:
  static char ID;
  TestExpr() : ModulePass(ID) { }

  virtual bool runOnModule(Module&);
};

#endif

