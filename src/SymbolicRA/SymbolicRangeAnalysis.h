#ifndef _SYMBOLICRANGEANALYSIS_H_
#define _SYMBOLICRANGEANALYSIS_H_

#include "Range.h"

#include "llvm/Pass.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/DominanceFrontier.h"

namespace llvm {

using std::vector;

class Junction;
class SymbolicRangeAnalysis;

raw_ostream& operator<<(raw_ostream& OS, const Junction& J);
raw_ostream& operator<<(raw_ostream& OS, const SymbolicRangeAnalysis& R);

/************
 * Junction *
 ************/
class Junction {
public:
  typedef vector<Junction*> ArgsTy;

  Junction(Value *Value);
  Junction(Value *Value, Range R);

  // RTTI for Junction.
  enum ValueId { ADD_ID, SUB_ID, MUL_ID, DIV_ID,
                 PHI_ID, SIGMA_ID, BASE_OP, NOOP_OP };
  virtual ValueId getValueId() const = 0;
  static bool classof(Junction const *) { return true; }

  ArgsTy::const_iterator begin() const { return Args_.begin(); }
  ArgsTy::const_iterator end()   const { return Args_.end(); }

  unsigned getIdx()   const { return Idx_; }
  Value   *getValue() const { return Value_; }

  virtual void printProlog(raw_ostream& OS) const = 0;
  virtual void printEpilog(raw_ostream& OS) const = 0;

  virtual void print(raw_ostream &OS)      const;
  virtual void printAsDot(raw_ostream &OS) const;

  virtual Junction *clone(Value *V, DenseMap<Junction*, Junction*> Subs) = 0;
  virtual Range eval()                                                   = 0;

  void     incIterations()       { Iterations_++; }
  unsigned getIterations() const { return Iterations_; }

  void  setRange(Range R);
  Range getRange()       const { return R_; }

  void      setArg(unsigned Idx, Junction *J);
  void      pushArg(Junction *J);
  bool      hasArg(Junction *J)  const;
  size_t    getNumArgs()         const;
  Junction *getArg(unsigned Idx) const;
  ArgsTy    getArgs()            const { return Args_; }

  void subs(DenseMap<Junction*, Junction*> Subs);

private:
  // Juction node counts for printing to dot graphs.
  Value *Value_;

  Range R_;
  unsigned Iterations_;

  static unsigned Idxs_;
  unsigned Idx_;

  ArgsTy Args_;

public:
  static unsigned EvalIdx_;
};

/****************
 * BaseJunction *
 ****************/
class BaseJunction : public Junction {
public:
  BaseJunction(Value *V, Expr Sym);

  // RTTI for Junction.
  virtual ValueId getValueId() const { return BASE_OP; }
  static bool classof(BaseJunction const *) { return true; }
  static bool classof(Junction const *J) {
    return J->getValueId() == BASE_OP;
  }

  Expr getSymbol() const { return Sym_; }

  virtual void printProlog(raw_ostream& OS) const { OS << "base"; }
  virtual void printEpilog(raw_ostream& OS) const { }

  virtual Junction *clone(Value *V, DenseMap<Junction*, Junction*>);
  virtual Range eval();

private:
  Expr Sym_;
};

/****************
 * NoopJunction *
 ****************/
class NoopJunction : public Junction {
public:
  NoopJunction(Value *V, Junction *J);

  // RTTI for Junction.
  virtual ValueId getValueId() const { return NOOP_OP; }
  static bool classof(NoopJunction const *) { return true; }
  static bool classof(Junction const *J) {
    return J->getValueId() == NOOP_OP;
  }

  virtual void printProlog(raw_ostream& OS) const { OS << "noop"; }
  virtual void printEpilog(raw_ostream& OS) const { }

  virtual Junction *clone(Value *V, DenseMap<Junction*, Junction*>);
  virtual Range eval();
};

/*****************
 * SigmaJunction *
 *****************/
class SigmaJunction : public Junction {
public:
  SigmaJunction(Value *V, unsigned Predicate, Junction *Bound,
                Junction *Incoming);

  // RTTI for PhiJunction.
  virtual ValueId getValueId() const { return PHI_ID; }
  static bool classof(SigmaJunction const*) { return true; }
  static bool classof(Junction const *J)  {
    return J->getValueId() == SIGMA_ID;
  }

  virtual void printProlog(raw_ostream& OS) const { OS << "sigma"; }
  virtual void printEpilog(raw_ostream& OS) const;

  virtual Junction *clone(Value *V, DenseMap<Junction*, Junction*> Subs);
  virtual Range eval();

  unsigned  getPredicate() const { return Predicate_; }
  Junction *getIncoming()  const { return getArg(0); }
  Junction *getBound()     const { return getArg(1); }

  Range getMeet();
  Range getEvalMeet();

private:
  unsigned Predicate_;
};

/***************
 * PhiJunction *
 ***************/
class PhiJunction : public Junction {
public:
  PhiJunction(Value *V);

  // RTTI for PhiJunction.
  virtual ValueId getValueId() const { return PHI_ID; }
  static bool classof(PhiJunction const*) { return true; }
  static bool classof(Junction const *J)  {
    return J->getValueId() == PHI_ID;
  }

  bool isWidened()  const { return Widened_; }
  void setWidened()       { Widened_ = true; }

  bool isNarrowed()  const { return Narrowed_; }
  void setNarrowed()       { Narrowed_ = true; }

  void pushIncoming(/* BasicBlock *IncomingBlock, */ Junction *Incoming);

  Range getMeet();
  Range getEvalMeet();

  virtual void printProlog(raw_ostream& OS) const { OS << "phi"; }
  virtual void printEpilog(raw_ostream& OS) const { }

  virtual Junction *clone(Value *V, DenseMap<Junction*, Junction*> Subs);
  virtual Range eval();

private:
  bool Widened_;
  bool Narrowed_;
};

/******************
 * BinaryJunction *
 ******************/
class BinaryJunction : public Junction {
public:
  BinaryJunction(Value *V, Junction *Left, Junction *Right);

  // RTTI for BinaryJunction.
  virtual ValueId getValueId() const = 0;
  static bool classof(BinaryJunction const *) { return true; }
  static bool classof(Junction const *J) {
    return J->getValueId() == ADD_ID ||
           J->getValueId() == SUB_ID ||
           J->getValueId() == MUL_ID ||
           J->getValueId() == DIV_ID;
  }

  Junction *getLeft()   const { return getArg(0); }
  Junction *getRight()  const { return getArg(1); }
};

/***************
 * AddJunction *
 ***************/
class AddJunction : public BinaryJunction {
public:
  AddJunction(Value *V, Junction *Left, Junction *Right);

  // RTTI for AddJunction.
  virtual ValueId getValueId() const { return ADD_ID; }
  static bool classof(AddJunction const*)    { return true; }
  static bool classof(BinaryJunction const*) { return true; }
  static bool classof(Junction const *J)  {
    return J->getValueId() == ADD_ID;
  }

  virtual void printProlog(raw_ostream& OS) const { OS << "add"; }
  virtual void printEpilog(raw_ostream& OS) const { }

  virtual Junction *clone(Value *V, DenseMap<Junction*, Junction*> Subs);
  virtual Range eval();
};

/***************
 * SubJunction *
 ***************/
class SubJunction : public BinaryJunction {
public:
  SubJunction(Value *V, Junction *Left, Junction *Right);

  // RTTI for AddJunction.
  virtual ValueId getValueId() const { return SUB_ID; }
  static bool classof(SubJunction const*)    { return true; }
  static bool classof(BinaryJunction const*) { return true; }
  static bool classof(Junction const *J)  {
    return J->getValueId() == SUB_ID;
  }

  virtual void printProlog(raw_ostream& OS) const { OS << "sub"; }
  virtual void printEpilog(raw_ostream& OS) const { }

  virtual Junction *clone(Value *V, DenseMap<Junction*, Junction*> Subs);
  virtual Range eval();
};

/***************
 * MulJunction *
 ***************/
class MulJunction : public BinaryJunction {
public:
  MulJunction(Value *V, Junction *Left, Junction *Right);

  // RTTI for AddJunction.
  virtual ValueId getValueId() const { return MUL_ID; }
  static bool classof(MulJunction const*)    { return true; }
  static bool classof(BinaryJunction const*) { return true; }
  static bool classof(Junction const *J)  {
    return J->getValueId() == MUL_ID;
  }

  virtual void printProlog(raw_ostream& OS) const { OS << "mul"; }
  virtual void printEpilog(raw_ostream& OS) const { }

  virtual Junction *clone(Value *V, DenseMap<Junction*, Junction*> Subs);
  virtual Range eval();
};

/***************
 * DivJunction *
 ***************/
class DivJunction : public BinaryJunction {
public:
  DivJunction(Value *V, Junction *Left, Junction *Right);

  // RTTI for AddJunction.
  virtual ValueId getValueId() const { return DIV_ID; }
  static bool classof(DivJunction const*)    { return true; }
  static bool classof(BinaryJunction const*) { return true; }
  static bool classof(Junction const *J)  {
    return J->getValueId() == DIV_ID;
  }

  virtual void printProlog(raw_ostream& OS) const { OS << "div"; }
  virtual void printEpilog(raw_ostream& OS) const { }

  virtual Junction *clone(Value *V, DenseMap<Junction*, Junction*> Subs);
  virtual Range eval();
};

/*************************
 * SymbolicRangeAnalysis *
 *************************/
class SymbolicRangeAnalysis : public ModulePass {
public:
  static char ID;
  SymbolicRangeAnalysis() : ModulePass(ID) { }

  typedef DenseMap<Value*, Junction*> JunctionsMapTy;

  JunctionsMapTy::const_iterator begin() const { return JunctionsMap_.begin(); }
  JunctionsMapTy::const_iterator end()   const { return JunctionsMap_.end();   }

  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual bool runOnModule(Module &M);

  virtual void print(raw_ostream& OS) const;
  void printAsDot(raw_ostream& OS) const;
  void printInfo(raw_ostream& OS) const;

  Range getRange(Value *V);

private:
  void setJunction(Value *V, Junction *J);
  void setSymbol(Value *V, Expr *Sym);

  Junction *getJunctionForValue(Value *V);

  void createConstraintsForFunction(Function *F);

  void createConstraintsForInst(Instruction *I);
  void createConstraintsForPhi(PHINode *Phi);
  void createConstraintsForTerminatorInst(TerminatorInst *TI);
  void createConstraintsOnBranchBlock(BasicBlock *BB, ICmpInst *ICI,
                                      bool DoSwap = false);

  void createJunctionsForFunctionArgs(Function *F);
  void createJunctionForInst(Instruction *I);

  AddJunction   *createAddJunction(BinaryOperator *BO);
  SubJunction   *createSubJunction(BinaryOperator *BO);
  MulJunction   *createMulJunction(BinaryOperator *BO);
  DivJunction   *createDivJunction(BinaryOperator *BO);
  BaseJunction  *createBaseJunction(Value *V);
  NoopJunction  *createNoopJunction(Value *V, Value *Op);
  SigmaJunction *createSigmaJunction(PHINode *Sigma, unsigned Predicate,
                                     Junction *Bound);
  PhiJunction   *createPhiJunction(Value *V);

  Function   *F_;
  BasicBlock *BB_;
  Value      *I_;

  DominatorTree *DT_;
  DominanceFrontier *DF_;

  DenseMap<Value*, Junction*> JunctionsMap_;
  DenseMap<Value*, Expr*> Symbols_;
};

} // end namespace llvm

#endif

