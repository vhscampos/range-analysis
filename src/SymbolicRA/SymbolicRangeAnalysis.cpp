//===--------------------- SymbolicRangeAnalysis.cpp ----------------------===//
//===----------------------------------------------------------------------===//

#include "SymbolicRangeAnalysis.h"
#include "Redefinition.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Timer.h"

#include <chrono>

/* ************************************************************************** */
/* ************************************************************************** */

namespace llvm {

raw_ostream& operator<<(raw_ostream& OS, const Junction& J) {
  J.print(OS);
  return OS;
}

raw_ostream& operator<<(raw_ostream& OS, const SymbolicRangeAnalysis& R) {
  R.print(OS);
  return OS;
}

} // end namespace llvm

/* ************************************************************************** */
/* ************************************************************************** */

using namespace llvm;
using std::make_pair;
using std::pair;
using std::string;
using std::vector;

static cl::opt<bool>
  ClDebug("sra-debug",
          cl::desc("Enable debugging for the symbolic range analysis"),
          cl::Hidden, cl::init(false));

static cl::opt<bool>
  ClDebugEval("sra-debug-eval",
              cl::desc("Enable debugging of the eval() functions for the "
                       "symbolic range analysis"),
              cl::Hidden, cl::init(false));

static cl::opt<bool>
  ClDebugConst("sra-debug-const",
               cl::desc("Enable debugging of constraint creation for the "
                        "symbolic range analysis"),
               cl::Hidden, cl::init(false));

static cl::opt<bool>
  ClStats("sra-stats",
          cl::desc("Collect and Show statistics for the symbolic range "
                   "analysis"),
          cl::Hidden, cl::init(false));

static cl::opt<string>
  ClDebugFunction("sra-debug-function",
                  cl::desc("Run and debug the symbolic range analysis "
                           "with a specific function"),
                  cl::Hidden, cl::init(""));

static RegisterPass<SymbolicRangeAnalysis>
  X("sra", "Constraint-graph-based sparse symbolic range analysis "
           "(requires GiNaC)", true, true);
char SymbolicRangeAnalysis::ID = 0;

#define RG_DEBUG(X)       { if (ClDebug) { X; }      }
#define RG_DEBUG_EVAL(X)  { if (ClDebugEval) { X; }  }
#define RG_DEBUG_CONST(X) { if (ClDebugConst) { X; } }
#define RG_ASSERT(X)      { if (!(X)) { printInfo(errs()); assert(X); } }

#define ASSERT_EQ(X, Y, M) __assert_eq(X, Y, M, __LINE__, __func__, __FILE__)
#define ASSERT_NE(X, Y, M) __assert_ne(X, Y, M, __LINE__, __func__, __FILE__)
#define ASSERT_TYPE(X, Y, M) __assert_type<Y>(X, #Y, M, __LINE__, __func__, __FILE__)

/* ************************************************************************** */
/* ************************************************************************** */

// assert_eq
template <class T>
static void __assert_eq(T Left, T Right, const char *Msg,
                        int Line, const char *Func, const char *File) {
  if (Left == Right)
    return;
  errs() << "Assertion failed: " << Left << " == " << Right << "\n";
  errs() << "                  " << Msg << "\n";
  assert(false);
}

// assert_ne
template <class T>
static void __assert_ne(T Left, T Right, const char *Msg,
                        int Line, const char *Func, const char *File) {
  if (Left != Right)
    return;
  errs() << "Assertion failed: " << Left << " != " << Right << "\n";
  errs() << "                  " << Msg << "\n";
  assert(false);
}

// __assert_type
template <typename T>
static void __assert_type(Value *V, const char *TypeRepr, const char *Msg,
                          int Line, const char *Func, const char *File) {
  if (isa<T>(V))
    return;
  errs() << "Assertion failed: isa<" << TypeRepr << ">(" << *V << ")\n";
  errs() << "                  " << Msg << "\n";
  assert(false);
}

// PrintValue
static void PrintValue(raw_ostream& OS, Value *V) {
  if (V->hasName())
    OS << V->getName();
  else
    OS << *V;
}

// IsRedefPhi
static bool IsRedefPhi(PHINode *Phi) {
  if (!Phi->hasName())
    return false;
  return Phi->getNumOperands() == 1 && Phi->getType()->isIntegerTy();
  //return Phi->getName().startswith(Redefinition::GetRedefPrefix());
}

// GetPredicateText
// From AsmWritef.cpp.
static const char *GetPredicateText(unsigned Predicate) {
  switch (Predicate) {
    case ICmpInst::ICMP_EQ:  return "eq";
    case ICmpInst::ICMP_NE:  return "ne";
    case ICmpInst::ICMP_SGT: return "sgt";
    case ICmpInst::ICMP_SGE: return "sge";
    case ICmpInst::ICMP_SLT: return "slt";
    case ICmpInst::ICMP_SLE: return "sle";
    case ICmpInst::ICMP_UGT: return "ugt";
    case ICmpInst::ICMP_UGE: return "uge";
    case ICmpInst::ICMP_ULT: return "ult";
    case ICmpInst::ICMP_ULE: return "ule";
    default:
      assert(false && "Unknown predicate");
  }
}

// GetMeet
Range GetMeet(Junction::ArgsTy Args) {
  Range R;
  for (auto& J : Args)
    if (J == *Args.begin())
      R = J->getRange();
    else
      R = R.meet(J->getRange());
  return R;
}

// GetEvalMeet
Range GetEvalMeet(Junction::ArgsTy Args) {
  Range R;
  for (auto& J : Args)
    if (J == *Args.begin())
      R = J->eval();
    else
      R = R.meet(J->eval());
  return R;
}

/* ************************************************************************** */
/* ************************************************************************** */

/***********
 * Junction *
 ***********/
Junction::Junction(Value *V)
  : Value_(V), R_(Range::GetBottomRange()), Iterations_(0), Idx_(Idxs_++) {
}

Junction::Junction(Value *V, Range R)
  : Value_(V), R_(R), Iterations_(0), Idx_(Idxs_++) {
}

// print
void Junction::print(raw_ostream &OS) const {
  Value *V = getValue();
  printProlog(OS);
  OS << "(";
  PrintValue(OS, V);
  OS << ")";
  ArgsTy::const_iterator J = begin(), E = end();
  if (J == E)
    return;
  OS << "[";
  for (; J != E; ++J) {
    if (J != begin())
      OS << " :: ";
    PrintValue(OS, (*J)->getValue());
  }
  OS << "]";
  printEpilog(OS);
}

// printAsDot
void Junction::printAsDot(raw_ostream &OS) const {
  OS << "  \"" << getIdx() << "\"";
  OS << " [label = \"" << *this << "\", color = \"cyan2\"]\n";
  for (auto& J : *this)
  OS << "  \"" << J->getIdx() << "\" -> \"" << getIdx()
     << "\" [color = \"cyan2\"]\n";
}

// setRange
void Junction::setRange(Range R) {
  R_ = R;
  if (R.getLower() == Expr::GetBottomValue() ||
      R.getLower() == Expr::GetBottomValue()) {
    ASSERT_EQ(R.getLower(), R.getUpper(), "Inconsistent state");
    // assert_eq(R.getLower() == R.getUpper() && "Inconsistent state");
  }
}

// pushArg
void Junction::pushArg(Junction *J) {
  Args_.push_back(J);
}

// getArg
Junction *Junction::getArg(unsigned Idx) const {
  return Args_[Idx];
}

// setArg
void Junction::setArg(unsigned Idx, Junction *J) {
  assert(Idx < getNumArgs() && "Invalid index specified");
  Args_[Idx] = J;
}

// hasArg
bool Junction::hasArg(Junction *J) const {
  return std::find(Args_.begin(), Args_.end(), J) != Args_.end();
}

// getNumArgs
size_t Junction::getNumArgs() const {
  return Args_.size();
}

// subs
void Junction::subs(DenseMap<Junction*, Junction*> Subs) {
  for (unsigned Idx = 0; Idx < getNumArgs(); ++Idx)
    if (Subs.count(getArg(Idx)))
      setArg(Idx, Subs[getArg(Idx)]);
}

unsigned Junction::Idxs_ = 0;
unsigned Junction::EvalIdx_ = 0;

/****************
 * BaseJunction *
 ****************/
BaseJunction::BaseJunction(Value *V, Expr Sym)
  : Junction(V, Range(Sym)), Sym_(Sym) {
}

// clone
Junction *BaseJunction::clone(Value *V, DenseMap<Junction*, Junction*>) {
  Junction *J = new BaseJunction(V, getSymbol());
  // TODO:
  // Why did I comment this?
  // J->setRange(getRange());
  return J;
}

Range BaseJunction::eval() {
  unsigned Idx = EvalIdx_++;
  RG_DEBUG_EVAL(dbgs() << "eval(): " << *this << ", " << Idx << "\n");
  return getRange();
}

/****************
 * NoopJunction *
 ****************/
NoopJunction::NoopJunction(Value *V, Junction *J)
  : Junction(V) {
  pushArg(J);
}

// clone
Junction *NoopJunction::clone(Value *V, DenseMap<Junction*, Junction*>) {
  return new NoopJunction(V, getArg(0));
}

// eval
Range NoopJunction::eval() {
  unsigned Idx = EvalIdx_++;
  RG_DEBUG_EVAL(dbgs() << "eval(): " << *this << ", " << Idx << "\n");

  Range R = getArg(0)->eval();

  RG_DEBUG_EVAL(dbgs() << "setRange(): " << R << ", " << Idx << "\n");
  setRange(R);
  return R;
}

/*****************
 * SigmaJunction *
 *****************/
SigmaJunction::SigmaJunction(Value *V, unsigned Predicate, Junction *Bound,
                             Junction *Incoming)
  : Junction(V), Predicate_(Predicate) {
  pushArg(Incoming);
  pushArg(Bound);
}

// printEpilog
void SigmaJunction::printEpilog(raw_ostream& OS) const {
  OS << " isect " << GetPredicateText(getPredicate()) << " " << *getBound();
}

// clone
Junction *SigmaJunction::clone(Value *V, DenseMap<Junction*, Junction*> Subs) {
  SigmaJunction *J = new SigmaJunction(V, getPredicate(), getBound(),
                                       getIncoming());
  J->subs(Subs);
  return J;
}

// eval
Range SigmaJunction::eval() {
  unsigned Idx = EvalIdx_++;
  RG_DEBUG_EVAL(dbgs() << "eval(): " << *this << ", " << Idx << "\n");

  Range R = getIncoming()->eval();

  if (R.getLower() == Expr::GetBottomValue())
    R.setLower(Expr::GetMinusInfValue());
  if (R.getUpper() == Expr::GetBottomValue())
    R.setUpper(Expr::GetPlusInfValue());

  // TODO: Explain.
  Range Bound = getBound()->eval();
  if (Bound.getLower() == Expr::GetBottomValue())
    Bound.setLower(Expr::GetMinusInfValue());
  if (Bound.getUpper() == Expr::GetBottomValue())
    Bound.setUpper(Expr::GetPlusInfValue());

  switch (getPredicate()) {
    case ICmpInst::ICMP_SLT:
    case ICmpInst::ICMP_ULT:
      R.setUpper(Bound.getUpper() - 1);
      break;
    case ICmpInst::ICMP_SLE:
    case ICmpInst::ICMP_ULE:
      R.setUpper(Bound.getUpper());
      break;
    case ICmpInst::ICMP_SGT:
    case ICmpInst::ICMP_UGT:
      R.setLower(Bound.getLower() + 1);
      break;
    case ICmpInst::ICMP_SGE:
    case ICmpInst::ICMP_UGE:
      R.setLower(Bound.getLower());
      break;
    case ICmpInst::ICMP_NE:
    case ICmpInst::ICMP_EQ:
      break;
    default:
      assert(false && "Invalid predicate");
  }

  RG_DEBUG_EVAL(dbgs() << "setRange(): " << R << ", " << Idx << "\n");
  setRange(R);
  return R;
}

/***************
 * PhiJunction *
 ***************/
PhiJunction::PhiJunction(Value *V)
  : Junction(V), Widened_(false), Narrowed_(false) {
}

// pushIncoming
void PhiJunction::pushIncoming(/* BasicBlock *IncomingBlock, */ Junction *Incoming) {
  pushArg(Incoming);
}

// clone
Junction *PhiJunction::clone(Value *V, DenseMap<Junction*, Junction*> Subs) {
  PhiJunction *J = new PhiJunction(V);
  for (unsigned Idx = 0; Idx < getNumArgs(); ++Idx)
    J->pushIncoming(/* IncomingBlocks_[Idx], */ getArg(Idx));
  J->subs(Subs);
  return J;
}

// eval
Range PhiJunction::eval() {
  unsigned Idx = EvalIdx_++;
  RG_DEBUG_EVAL(dbgs() << "eval(): " << *this << ", " << Idx << "\n");

  if (isNarrowed()) {
    RG_DEBUG_EVAL(dbgs() << "        isWidened()\n");
    return getRange();
  }

  bool ShouldStop   = getIterations() >= 3;
  bool ShouldWiden  = getIterations() == 1;
  bool ShouldNarrow = getIterations() == 0;

  // If we've already gone through all iterations, use the available
  // junction ranges instead of evaluating them again.
  // If we haven't completed the iterations, evaluate the arguments.
  incIterations();
  ArgsTy Args = getArgs();
  Range R = ShouldStop ? GetMeet(Args) : GetEvalMeet(Args);

  if (ShouldWiden) {
    R = R.widen(getRange());
    setWidened();
  } else if (ShouldNarrow) {
    ArgsTy Args = getArgs();
    R = GetEvalMeet(Args);
    setNarrowed();
  } else {
    if (R.getLower() == Expr::GetBottomValue()) {
      ASSERT_EQ(R.getUpper(), Expr::GetBottomValue(),
                "Inconsistent state");
      R.setLower(Expr::GetMinusInfValue());
      R.setUpper(Expr::GetPlusInfValue());
    }
  }

  RG_DEBUG_EVAL(dbgs() << "setRange(): " << R << ", " << Idx << "\n");
  ASSERT_NE(R.getLower(), Expr::GetBottomValue(),
            "Unevaluated lower bound");
  ASSERT_NE(R.getUpper(), Expr::GetBottomValue(),
            "Unevaluated upper bound");
  setRange(R);
  return R;
}

/******************
 * BinaryJunction *
 ******************/
BinaryJunction::BinaryJunction(Value *V,
                               Junction *Left, Junction *Right)
  : Junction(V) {
  pushArg(Left);
  pushArg(Right);
}

/***************
 * AddJunction *
 ***************/
AddJunction::AddJunction(Value *V, Junction *Left, Junction *Right)
  : BinaryJunction(V, Left, Right) {
}

// clone
Junction *AddJunction::clone(Value *V, DenseMap<Junction*, Junction*> Subs) {
  AddJunction *J = new AddJunction(V, getLeft(), getRight());
  J->subs(Subs);
  return J;
}

// eval
Range AddJunction::eval() {
  unsigned Idx = EvalIdx_++;
  RG_DEBUG_EVAL(dbgs() << "eval(): " << *this << ", " << Idx << "\n");

  Junction *Left  = getLeft();
  Junction *Right = getRight();

  Range LeftEval  = Left->eval();
  Range RightEval = Right->eval();

  Range R = LeftEval + RightEval;

  RG_DEBUG_EVAL(dbgs() << "setRange(): " << R << ", " << Idx << "\n");
  setRange(R);
  return R;
}

/***************
 * SubJunction *
 ***************/
SubJunction::SubJunction(Value *V, Junction *Left, Junction *Right)
  : BinaryJunction(V, Left, Right) {
}

// clone
Junction *SubJunction::clone(Value *V, DenseMap<Junction*, Junction*> Subs) {
  SubJunction *J = new SubJunction(V, getLeft(), getRight());
  J->subs(Subs);
  return J;
}

// eval
Range SubJunction::eval() {
  unsigned Idx = EvalIdx_++;
  RG_DEBUG_EVAL(dbgs() << "eval(): " << *this << ", " << Idx << "\n");

  Junction *Left  = getLeft();
  Junction *Right = getRight();

  Range LeftEval  = Left->eval();
  Range RightEval = Right->eval();

  Range R = LeftEval - RightEval;

  RG_DEBUG_EVAL(dbgs() << "setRange(): " << R << ", " << Idx << "\n");
  setRange(R);
  return R;
}

/***************
 * MulJunction *
 ***************/
MulJunction::MulJunction(Value *V, Junction *Left, Junction *Right)
  : BinaryJunction(V, Left, Right) {
}

// clone
Junction *MulJunction::clone(Value *V, DenseMap<Junction*, Junction*> Subs) {
  MulJunction *J = new MulJunction(V, getLeft(), getRight());
  J->subs(Subs);
  return J;
}

// eval
Range MulJunction::eval() {
  unsigned Idx = EvalIdx_++;
  RG_DEBUG_EVAL(dbgs() << "eval(): " << *this << ", " << Idx << "\n");

  Junction *Left  = getLeft();
  Junction *Right = getRight();

  Range LeftEval  = Left->eval();
  Range RightEval = Right->eval();

  Range R = LeftEval * RightEval;

  RG_DEBUG_EVAL(dbgs() << "setRange(): " << R << ", " << Idx << "\n");
  setRange(R);
  return R;
}

/***************
 * DivJunction *
 ***************/
DivJunction::DivJunction(Value *V, Junction *Left, Junction *Right)
  : BinaryJunction(V, Left, Right) {
}

// clone
Junction *DivJunction::clone(Value *V, DenseMap<Junction*, Junction*> Subs) {
  DivJunction *J = new DivJunction(V, getLeft(), getRight());
  J->subs(Subs);
  return J;
}

// eval
Range DivJunction::eval() {
  unsigned Idx = EvalIdx_++;
  RG_DEBUG_EVAL(dbgs() << "eval(): " << *this << ", " << Idx << "\n");

  Junction *Left  = getLeft();
  Junction *Right = getRight();

  Range LeftEval  = Left->eval();
  Range RightEval = Right->eval();

  Range R = LeftEval/RightEval;

  RG_DEBUG_EVAL(dbgs() << "setRange(): " << R << ", " << Idx << "\n");
  setRange(R);
  return R;
}

/*************************
 * SymbolicRangeAnalysis *
 *************************/
// getAnalysisUsage
void SymbolicRangeAnalysis::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<DominatorTree>();
  AU.addRequired<DominanceFrontier>();
  AU.setPreservesAll();
}

// runOnModule
bool SymbolicRangeAnalysis::runOnModule(Module& M) {
  auto Start = std::chrono::high_resolution_clock::now();

  bool OrClDebug = ClDebug;
  bool OrClDebugEval = ClDebugEval;
  bool OrClDebugConst = ClDebugConst;

  for (auto& F : M) {
    if (F.isIntrinsic() || F.isDeclaration())
      continue;

    if (!ClDebugFunction.empty()) {
      if (F.getName() != ClDebugFunction) {
        ClDebug = false;
        ClDebugEval = false;
        ClDebugConst = false;
      } else {
        ClDebug = true;
        ClDebugEval = true;
        ClDebugConst = true;
      }
    }

    createJunctionsForFunctionArgs(&F);
  }

  for (auto& F : M) {
    if (F.isIntrinsic() || F.isDeclaration())
      continue;
    if (!ClDebugFunction.empty()) {
      if (F.getName() != ClDebugFunction) {
        ClDebug = false;
        ClDebugEval = false;
        ClDebugConst = false;
      } else {
        ClDebug = true;
        ClDebugEval = true;
        ClDebugConst = true;
      }
    }

    DT_ = &getAnalysis<DominatorTree>(F);
    DF_ = &getAnalysis<DominanceFrontier>(F);

    createConstraintsForFunction(&F);
  }

  ClDebug = OrClDebug;
  ClDebugEval = OrClDebugEval;
  ClDebugConst = OrClDebugConst;

  auto End = std::chrono::high_resolution_clock::now();
  auto Elapsed = std::chrono::duration_cast<std::chrono::milliseconds>
                   (End - Start).count();

  dbgs() << "================= SRA STATS =================\n";
  dbgs() << "==== Total evaluation time:     " << Elapsed << "\t ====\n";
  dbgs() << "==== Number of junctions:       " << JunctionsMap_.size() << "\t ====\n";

  if (ClDebug) {
    dbgs() << "================= SRA DEBUG =================\n";
    dbgs() << *this;
  }

  if (ClStats) {
    dbgs() << "================= SRA STATS =================\n";
    dbgs() << "==== Number of junctions:       "
           << JunctionsMap_.size() << "\t ====\n";

    for (auto& J : JunctionsMap_) {
      Range R = J.second->eval();
      ASSERT_NE(R.getLower(), Expr::GetBottomValue(),
                "Unevaluated lower bound");
      ASSERT_NE(R.getUpper(), Expr::GetBottomValue(),
                "Unevaluated upper bound");
    }
  }

  return false;
}

// print
void SymbolicRangeAnalysis::print(raw_ostream& OS) const {
  std::vector<pair<Junction*, Range> > Ranges;
  for (auto& J : JunctionsMap_) {
    if (!ClDebugFunction.empty())
      if (Instruction *I = dyn_cast<Instruction>(J.first)) {
        if (I->getParent()->getParent()->getName() != ClDebugFunction)
          continue;
      } else if (Argument *A = dyn_cast<Argument>(J.first)) {
        if (A->getParent()->getName() != ClDebugFunction)
          continue;
      } else if (isa<Constant>(J.first)) {
        continue;
      }

    RG_DEBUG_EVAL(dbgs() << "==== eval() ==== " << *J.second << " ====\n");
    Ranges.push_back(make_pair(J.second, J.second->eval()));
  }

  OS << "=================== RANGES ==================\n";
  for (auto& P : Ranges) {
    OS << *P.first<< " = " << P.second << "\n";
  }
}

// printDot
void SymbolicRangeAnalysis::printAsDot(raw_ostream& OS) const {
  OS << "digraph module {\n";
  OS << "  graph [rankdir = LR, margin = 0];\n";
  OS << "  node [shape = record,fontname = \"Times-Roman\", fontsize = 14];\n";
  for (auto& J : JunctionsMap_)
    J.second->printAsDot(OS);
  OS << "}\n";
}

// printInfo
void SymbolicRangeAnalysis::printInfo(raw_ostream& OS) const {
  OS << "## Function:    " << F_->getName()  << "\n";
  OS << "## Block:       " << BB_->getName() << "\n";
  OS << "## Instruction: " << I_->getName()  << "\n";
}

// getRange
Range SymbolicRangeAnalysis::getRange(Value *V) {
  if (JunctionsMap_.count(V))
    return JunctionsMap_[V]->eval();
  else if (isa<ConstantInt>(V))
    return Range(V);
  return Range::GetInfRange();
}

// setJunction
void SymbolicRangeAnalysis::setJunction(Value *V, Junction *J) {
  JunctionsMap_[V] = J;
}

// setSymbol
void SymbolicRangeAnalysis::setSymbol(Value *V, Expr *Sym) {
  Symbols_[V] = Sym;
}

// getJunctionForValue
Junction *SymbolicRangeAnalysis::getJunctionForValue(Value *V) {
  assert(V->getType()->isIntegerTy() && "Invalid non-integer value");
  auto It = JunctionsMap_.find(V);
  if (It == JunctionsMap_.end()) {
    ASSERT_TYPE(V, Constant, "Uninitialized value is not a constant");
    return createBaseJunction(V);
  }
  return It->second;
}

// createConstraintsForFunction
void SymbolicRangeAnalysis::createConstraintsForFunction(Function *F) {
  F_ = F;

  for (auto& BB : *F) {
    BB_ = &BB;
    for (auto& I : BB)
      createJunctionForInst(&I);

    TerminatorInst *TI = BB.getTerminator();
    createConstraintsForTerminatorInst(TI);
  }

  for (auto& BB : *F)
    for (auto& I : BB)
      createConstraintsForInst(&I);
}

// createConstraintsForInst
void SymbolicRangeAnalysis::createConstraintsForInst(Instruction *I) {
  RG_DEBUG_CONST(dbgs() << "createConstraintsForInst: " << *I << "\n");
  if (!I->getType()->isIntegerTy())
    return;
  I_ = I;
  unsigned Opcode = I->getOpcode();
  switch (Opcode) {
    case Instruction::PHI:
      if (!IsRedefPhi(cast<PHINode>(I)))
        createConstraintsForPhi(cast<PHINode>(I));
      break;
    default:
      break;
  }
}

// createConstraintsForPhi
void SymbolicRangeAnalysis::createConstraintsForPhi(PHINode *Phi) {
  RG_DEBUG_CONST(dbgs() << "createConstraintsForPhi: " << *Phi << "\n");
  if (!Phi->getType()->isIntegerTy())
    return;
  PhiJunction *J = cast<PhiJunction>(getJunctionForValue(Phi));
    for (unsigned Idx = 0; Idx < Phi->getNumIncomingValues(); ++Idx) {
      // BasicBlock *IncomingBlock = Phi->getIncomingBlock(Idx);
      Junction *Incoming = getJunctionForValue(Phi->getIncomingValue(Idx));
      J->pushIncoming(/* IncomingBlock, */ Incoming);
    }
}

// createConstraintsForTerminatorInst
void SymbolicRangeAnalysis::createConstraintsForTerminatorInst(TerminatorInst *TI) {
  BranchInst *BI = dyn_cast<BranchInst>(TI);
  if (!BI || BI->isUnconditional())
    return;

  ICmpInst *ICI = dyn_cast<ICmpInst>(BI->getCondition());
  if (!ICI || !ICI->getOperand(0)->getType()->isIntegerTy())
    return;

  RG_DEBUG_CONST(dbgs() << "createConstraintsForTerminatorInst: " << *BI << "\n");

  createConstraintsOnBranchBlock(BI->getSuccessor(0), ICI);
  if (DT_->dominates(TI->getParent(), BI->getSuccessor(1)))
    createConstraintsOnBranchBlock(BI->getSuccessor(1), ICI, true);
}

// createConstraintsOnBranchBlock
void SymbolicRangeAnalysis::createConstraintsOnBranchBlock(BasicBlock *BB,
                                               ICmpInst *ICI, bool DoSwap) {
  unsigned NumFoundSigmas = 0;
  Value *FoundSigmaForIncoming = NULL;

  DenseMap<Junction*, Junction*> Subs;

  unsigned Predicate = DoSwap ? ICI->getInversePredicate()
                              : ICI->getPredicate();
  unsigned SwappedPredicate = DoSwap ? ICI->getSwappedPredicate()
                                     : ICI->getInversePredicate();

  for (auto& I : *BB) {
    PHINode *Phi = dyn_cast<PHINode>(&I);
    if (!Phi)
      break;

    if (IsRedefPhi(Phi)) {
      Junction *New;

      Value *Incoming = Phi->getIncomingValue(0);
      Junction *J = getJunctionForValue(Incoming);

      // Only create sigma nodes for the branch operands.
      // Left-hand side operand of the comparison.
      if (Incoming == ICI->getOperand(0)) {
        RG_ASSERT(FoundSigmaForIncoming != Incoming &&
                  "More than one definition for the same sigma");
        assert(NumFoundSigmas < 2 && "Extranius sigma redefinitions");
        ++NumFoundSigmas;
        FoundSigmaForIncoming = Incoming;

        Junction *Bound = getJunctionForValue(ICI->getOperand(1));
        New = createSigmaJunction(Phi, Predicate, Bound);
      }
      // Right-hand side operand of the comparison.
      else if (Incoming == ICI->getOperand(1)) {
        assert(FoundSigmaForIncoming != Incoming &&
               "More than one definition for the same sigma");
        assert(NumFoundSigmas < 2 && "Extranius sigma redefinitions");
        ++NumFoundSigmas;
        FoundSigmaForIncoming = Incoming;

        Junction *Bound = getJunctionForValue(ICI->getOperand(0));
        New = createSigmaJunction(Phi, SwappedPredicate, Bound);
      }
      // For value that are not branch operands, clone the incoming value's
      // junction, replacing it's value with the phi node and replacing
      // it's arguments with the redefinitions in Subs, if any.
      else
        New = J->clone(Phi, Subs);

      Subs.insert(std::make_pair(J, New));
      setJunction(Phi, New);
    }
  }
}

// createArgumentJunctionsForFunction
void SymbolicRangeAnalysis::createJunctionsForFunctionArgs(Function* F) {
  RG_DEBUG_CONST(dbgs() << "createJunctionsForFunctionArgs: " << F->getName()
                  << "\n");

  if (F->getName() == "main" && F->getFunctionType()->getNumParams() == 2) {
    auto AI = F->arg_begin();
    BaseJunction *J = createBaseJunction(&(*AI));
    J->setRange(Range(Expr(0), J->getRange().getUpper()));
    return;
  }

  for (auto AI = F->arg_begin(), E = F->arg_end(); AI != E; ++AI) {
    Argument *A = &(*AI);
    I_ = A;
    if (A->getType()->isIntegerTy())
      createPhiJunction(A);
  }
}

// createJunctionForInst
void SymbolicRangeAnalysis::createJunctionForInst(Instruction *I) {
  if (CallInst *CI = dyn_cast<CallInst>(I)) {
    if (Function *F = CI->getCalledFunction()) {
      if (!F->isIntrinsic() && !F->isDeclaration())
        //errs() << "F: " << F->getName() << "\n";
        for (auto AI = F->arg_begin(), AE = F->arg_end(); AI != AE; ++AI) {
          if (!AI->getType()->isIntegerTy())
            continue;
          PhiJunction *P = cast<PhiJunction>(getJunctionForValue(&(*AI)));
          //if (!P)
          //  continue;
          Value *Real = CI->getArgOperand(AI->getArgNo());
          //errs() << "Pushing: " << *Real << " for " << *AI << "\n";
          P->pushIncoming(getJunctionForValue(Real));
        }
    }
  }
  if (!I->getType()->isIntegerTy())
    return;
  RG_DEBUG_CONST(dbgs() << "createJunctionForInst: " << *I << "\n");
  unsigned Predicate = I->getOpcode();
  switch (Predicate) {
    case Instruction::Add:
      createAddJunction(cast<BinaryOperator>(I));
      break;
    case Instruction::Sub:
      createSubJunction(cast<BinaryOperator>(I));
      break;
    case Instruction::Mul:
      createMulJunction(cast<BinaryOperator>(I));
      break;
    case Instruction::SDiv:
    case Instruction::UDiv:
      createDivJunction(cast<BinaryOperator>(I));
      break;
    case Instruction::BitCast:
    case Instruction::SExt:
      createNoopJunction(I, I->getOperand(0));
      break;
    case Instruction::PHI:
      if (!IsRedefPhi(cast<PHINode>(I)))
        createPhiJunction(cast<PHINode>(I));
      break;
    default:
      createBaseJunction(I);
  }
}

// createAddJunction
AddJunction *SymbolicRangeAnalysis::createAddJunction(BinaryOperator *BO) {
  RG_DEBUG_CONST(dbgs() << "createAddJunction: " << *BO << "\n");

  Value *LeftVal  = BO->getOperand(0);
  Value *RightVal = BO->getOperand(1);

  Junction *Left   = getJunctionForValue(LeftVal);
  Junction *Right  = getJunctionForValue(RightVal);

  AddJunction *J = new AddJunction(BO, Left, Right);
  setJunction(BO, J);

  return J;
}

// createSubJunction
SubJunction *SymbolicRangeAnalysis::createSubJunction(BinaryOperator *BO) {
  RG_DEBUG_CONST(dbgs() << "createSubJunction: " << *BO << "\n");

  Value *LeftVal  = BO->getOperand(0);
  Value *RightVal = BO->getOperand(1);

  Junction *Left   = getJunctionForValue(LeftVal);
  Junction *Right  = getJunctionForValue(RightVal);

  SubJunction *J = new SubJunction(BO, Left, Right);
  setJunction(BO, J);

  return J;
}

// createMulJunction
MulJunction *SymbolicRangeAnalysis::createMulJunction(BinaryOperator *BO) {
  RG_DEBUG_CONST(dbgs() << "createMulJunction: " << *BO << "\n");

  Value *LeftVal  = BO->getOperand(0);
  Value *RightVal = BO->getOperand(1);

  Junction *Left   = getJunctionForValue(LeftVal);
  Junction *Right  = getJunctionForValue(RightVal);

  MulJunction *J = new MulJunction(BO, Left, Right);
  setJunction(BO, J);

  return J;
}

// createDivJunction
DivJunction *SymbolicRangeAnalysis::createDivJunction(BinaryOperator *BO) {
  RG_DEBUG_CONST(dbgs() << "createDivJunction: " << *BO << "\n");

  Value *LeftVal  = BO->getOperand(0);
  Value *RightVal = BO->getOperand(1);

  Junction *Left   = getJunctionForValue(LeftVal);
  Junction *Right  = getJunctionForValue(RightVal);

  DivJunction *J = new DivJunction(BO, Left, Right);
  setJunction(BO, J);

  return J;
}

// createBaseJunction
BaseJunction *SymbolicRangeAnalysis::createBaseJunction(Value *V) {
  RG_DEBUG_CONST(dbgs() << "createBaseJunction: " << *V << "\n");

  Expr *ConstOrSym = new Expr(V);
  if (!isa<ConstantInt>(V))
    setSymbol(V, ConstOrSym);
  BaseJunction *J = new BaseJunction(V, *ConstOrSym);
  setJunction(V, J);
  return J;
}

// createNoopJunction
NoopJunction *SymbolicRangeAnalysis::createNoopJunction(Value *V, Value *Op) {
  RG_DEBUG_CONST(dbgs() << "createNoopJunction: " << *V << " :: " << *Op << "\n");

  Junction *Incoming = getJunctionForValue(Op);
  NoopJunction *J = new NoopJunction(V, Incoming);
  setJunction(V, J);
  return J;
}

// createSigmaJunction
SigmaJunction *SymbolicRangeAnalysis::createSigmaJunction(PHINode *Sigma,
                                  unsigned Predicate, Junction *Bound) {
  RG_DEBUG_CONST(dbgs() << "createSigmaJunction: " << *Sigma << " :: " << Predicate
                  << " :: " << *Bound << "\n");

  // TODO:
  Junction *Incoming = getJunctionForValue(Sigma->getIncomingValue(0));
  SigmaJunction *J = new SigmaJunction(Sigma, Predicate, Bound, Incoming);
  setJunction(Sigma, J);
  return J;
}

// createPhiJunction
PhiJunction *SymbolicRangeAnalysis::createPhiJunction(Value *V) {
  RG_DEBUG_CONST(dbgs() << "createPhiJunction: " << *V << "\n");

  PhiJunction *J = new PhiJunction(V);
  setJunction(V, J);

  return J;
}

/*****************
 * AnnotateIntSizes *
 *****************/
class AnnotateIntSizes : public ModulePass {
public:
  static char ID;
  AnnotateIntSizes() : ModulePass(ID) { }

  virtual void getAnalysisUsage(AnalysisUsage& AU) const;
  virtual bool runOnModule(Module& M);

private:
  void setMetadataOn(Instruction *I, Range R); 

  LLVMContext *Context_;

  SymbolicRangeAnalysis *SRA_;
};

void AnnotateIntSizes::getAnalysisUsage(AnalysisUsage& AU) const {
  AU.addRequired<SymbolicRangeAnalysis>();
  AU.setPreservesAll();
}

bool AnnotateIntSizes::runOnModule(Module& M) {
  Context_ = &M.getContext();

  SRA_ = &getAnalysis<SymbolicRangeAnalysis>();

  for (auto& J : *SRA_)
    if (Instruction *I = dyn_cast<Instruction>(J.first))
      setMetadataOn(I, J.second->eval());
    //else if (Argument *A = dyn_cast<Argument>(J.first))
    //  errs() << "Argument size: " << *A << ", " << A->getParent()->getName()
    //         << " == " << J.second->eval() << "\n";

  return false;
}

void AnnotateIntSizes::setMetadataOn(Instruction *I, Range R) { 
  string Str = string("[") + R.getLower().getStringRepr() + ", " +
               R.getUpper().getStringRepr() + "]";
  MDString *MDStr = MDString::get(*Context_, Str);
  I->setMetadata("isize", MDNode::get(*Context_, ArrayRef<Value*>(MDStr)));
}

char AnnotateIntSizes::ID = 0;
static RegisterPass<AnnotateIntSizes>
  S("sra-annotate-sizes", "Annotate pointer sizes infered by the "
    "symbolic range analysis", false, false);

