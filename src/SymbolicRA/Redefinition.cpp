//===--------------------- SymbolicRangeAnalysis.cpp ----------------------===//
//===----------------------------------------------------------------------===//

#include "Redefinition.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"

#include <queue>

/* ************************************************************************** */
/* ************************************************************************** */

using namespace llvm;
using std::queue;
using std::set;
using std::string;

static cl::opt<bool>
  ClDebug("redef-debug",
          cl::desc("Enable debugging for the integer live-range splitting"),
          cl::Hidden, cl::init(false));

static cl::opt<string>
  ClDebugFunction("redef-debug-function",
                  cl::desc("Run and debug integer live-range splitting "
                           "with a specific function"),
                  cl::Hidden, cl::init(""));

static cl::opt<bool>
  ClStats("redef-stats",
          cl::desc("Show statistics for the integer live-range splitting"),
          cl::Hidden, cl::init(false));

char Redefinition::ID = 0;
static RegisterPass<Redefinition>
  X("redef", "Integer live-range splitting");

#define RDEF_DEBUG(X) { if (ClDebug) { X; } }

/* ************************************************************************** */
/* ************************************************************************** */

// IsRedefinable
// Integer values are redefinable if they're arguments or binary operators.
static bool IsRedefinable(Value *V) {
  return V->getType()->isIntegerTy() && !isa<Constant>(V); // isa<BinaryOperator>(V) ||
         // (V->getType()->isIntegerTy() && isa<Argument>(V));
}

// GetTransitiveRedefinitions
// Returns the transitive closure of uses of the given variable that are
// redefinable. The instructions must dominate BB.
static void
  GetTransitiveRedefinitions(Value *V, BasicBlock *BB, DominatorTree *DT,
                               set<Value*>& Redefinitions) {
  assert(IsRedefinable(V) && "Parameter must be redefinable");

  queue<Value*> Uses;
  // Initialize the worklist with the given value.
  Uses.push(V);

  while (!Uses.empty()) {
    Value *U = Uses.front();
    assert(IsRedefinable(U) && "Value must be redefinable");
    Uses.pop();

    // Skip the visited values.
    if (Redefinitions.count(U))
      continue;

    // Only redefine if the definition dominates the redefinition site.
    if (Instruction *I = dyn_cast<Instruction>(U))
      if (I->getParent() == BB || !DT->dominates(I->getParent(), BB))
        continue;

    // Insert the value into the redefinition list.
    Redefinitions.insert(U);

    // Add all its redefinable uses to the worklist.
    for (auto UI = U->use_begin(), UE = U->use_end(); UI != UE; ++UI)
      if (IsRedefinable(*UI))
        Uses.push(*UI);
  }
}

// CreateNamedPhi
static PHINode *CreateNamedPhi(Value *V, Twine Prefix,
                               BasicBlock::iterator Position) {
  Twine Name = Prefix;
  if (V->hasName())
    Name = Prefix + "." + V->getName();
  return PHINode::Create(V->getType(), 1, Name, Position);
}

/* ************************************************************************** */
/* ************************************************************************** */

// getAnalysisUsage
void Redefinition::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<DominatorTree>();
  AU.addRequired<DominanceFrontier>();
  AU.setPreservesCFG();
}

// runOnModule
bool Redefinition::runOnFunction(Function &F) {
  if (!ClDebugFunction.empty() && F.getName() != ClDebugFunction)
    return false;

  DT_ = &getAnalysis<DominatorTree>();
  DF_ = &getAnalysis<DominanceFrontier>();

  createSigmasInFunction(&F);

  return true;
}

// doFinalization
bool Redefinition::doFinalization(Module& M) {
  dbgs() << "=================== STATS ===================\n"; 
  dbgs() << "==== Number of create sigmas:   "
         << StatNumCreatedSigmas_       << "\t ====\n";
  dbgs() << "==== Number of create phis:     "
         << StatNumCreatedFrontierPhis_ << "\t ====\n";
  dbgs() << "==== Number of instructions:    "
         << StatNumInstructions_        << "\t ====\n";
  return true;
}

// createSigmasInFunction
// Create sigma nodes for all branches in the function.
void Redefinition::createSigmasInFunction(Function *F) {
  for (auto& BB : *F) {
    StatNumInstructions_ += BB.getInstList().size();
    // Rename operands used in conditional branches and their dependencies.
    TerminatorInst *TI = BB.getTerminator();
    if (BranchInst *BI = dyn_cast<BranchInst>(TI))
      if (BI->isConditional())
        createSigmasForCondBranch(BI);
  }
}

// createSigmasForCondBranch
void Redefinition::createSigmasForCondBranch(BranchInst *BI) {
  assert(BI->isConditional() && "Expected conditional branch");

  ICmpInst *ICI = dyn_cast<ICmpInst>(BI->getCondition());
  if (!ICI || !ICI->getOperand(0)->getType()->isIntegerTy())
    return;

  RDEF_DEBUG(dbgs() << "createSigmasForCondBranch: " << *BI << "\n");

  Value *Left = ICI->getOperand(0);
  Value *Right = ICI->getOperand(1);

  BasicBlock *TB = BI->getSuccessor(0);
  BasicBlock *FB = BI->getSuccessor(1);

  bool HasSinglePredTB = TB->getSinglePredecessor() != NULL;
  bool HasSinglePredFB = FB->getSinglePredecessor() != NULL;

  bool IsRedefinableRight = IsRedefinable(Right);
  bool SameBranchOperands = Left == Right;

  //errs() << HasSinglePredTB << ", " << HasSinglePredFB << "\n";
  //errs() << IsRedefinable(Left) << ", " << IsRedefinableRight << "\n";

  if (IsRedefinable(Left)) {
    // We don't want to place extranius definitions of a value, so the only
    // place the sigma once if the branch operands are the same.
    Value *Second = SameBranchOperands && IsRedefinableRight ? Right : NULL;
    if (HasSinglePredTB)
      createSigmaNodesForValueAt(Left, Second, TB);
    if (HasSinglePredFB)
      createSigmaNodesForValueAt(Left, Second, FB);
  }
  // Left isn't definable down here.
  else if (IsRedefinable(Right)) {
    if (HasSinglePredTB)
      createSigmaNodesForValueAt(Right, NULL, TB);
    if (HasSinglePredFB)
      createSigmaNodesForValueAt(Right, NULL, FB);
  }
}

// createSigmaNodesForValueAt
// Creates sigma nodes the value and the transitive closure of its dependencies.
// To avoid extra redefinitions, we pass in both branch values so that the
// and use the union of both redefinition sets.
void Redefinition::createSigmaNodesForValueAt(Value *V, Value *C,
                                              BasicBlock *BB) {
  assert(BB->getSinglePredecessor() && "Block has multiple predecessors");

  RDEF_DEBUG(dbgs() << "createSigmaNodesForValueAt: " << *V << " :: "
                    << *C << " :: " << BB->getName() << "\n");

  set<Value*> Redefinitions; 
  GetTransitiveRedefinitions(V, BB, DT_, Redefinitions);
  if (C)
    GetTransitiveRedefinitions(C, BB, DT_, Redefinitions);

  auto Position = BB->begin();
  while (isa<PHINode>(&(*Position)))
    Position++;

  for (auto& S : Redefinitions)
    createSigmaNodeForValueAt(S, BB, Position);
}

void Redefinition::createSigmaNodeForValueAt(Value *V, BasicBlock *BB,
                                             BasicBlock::iterator Position) {
  RDEF_DEBUG(dbgs() << "createSigmaNodeForValueAt: " << *V << "\n");

  PHINode *BranchRedef = CreateNamedPhi(V, GetRedefPrefix(), Position);
  BranchRedef->addIncoming(V, BB->getSinglePredecessor());
  StatNumCreatedSigmas_++;

  // Phi nodes should be created on all blocks in the dominance frontier of BB
  // where V is defined.
  auto DI = DF_->find(BB);
  if (DI != DF_->end())
    for (auto& BI : DI->second)
      // If the block in the frontier dominates a use of V, then a phi node
      // should be created at said block.
      if (dominatesUse(V, BI))
         if (PHINode *FrontierRedef = createPhiNodeAt(V, BI))
           // Replace all incoming definitions with the omega node for every
           // predecessor where the omega node is defined.
           for (auto PI = pred_begin(BI), PE = pred_end(BI); PI != PE; ++PI)
             if (DT_->dominates(BB, *PI)) {
               FrontierRedef->removeIncomingValue(*PI);
               FrontierRedef->addIncoming(BranchRedef, *PI);
             }

  // Replace all users of the V with the new sigma, starting at BB.
  replaceUsesOfWithAfter(V, BranchRedef, BB);
}

// createPhiNodeAt
// Creates a phi node for the given value at the given block.
PHINode *Redefinition::createPhiNodeAt(Value *V, BasicBlock *BB) {
  assert(!V->getType()->isPointerTy() && "Value must not be a pointer");

  RDEF_DEBUG(dbgs() << "createPhiNodeAt: " << *V << " at "
                     << BB->getName() << "\n");

  // Return NULL if V isn't defined on all predecessors of BB.
  if (Instruction *I = dyn_cast<Instruction>(V))
    for (pred_iterator PI = pred_begin(BB), PE = pred_end(BB); PI != PE; ++PI)
      if (!DT_->dominates(I->getParent(), *PI))
        return NULL;

  PHINode *Phi = CreateNamedPhi(V, GetPhiPrefix(), BB->begin());

  // Add the default incoming values.
  for (pred_iterator PI = pred_begin(BB), PE = pred_end(BB); PI != PE; ++PI)
    Phi->addIncoming(V, *PI);

  // Replace all uses of V with the phi node, starting at BB.
  replaceUsesOfWithAfter(V, Phi, BB);

  StatNumCreatedFrontierPhis_++;

  return Phi;
}

// dominatesUse
// Returns true if BB dominates a use of V.
bool Redefinition::dominatesUse(Value *V, BasicBlock *BB) {
  for (auto UI = V->use_begin(), UE = V->use_end(); UI != UE; ++UI)
    // A phi node can dominate its operands, so they have to be disregarded.
    if (isa<PHINode>(*UI) || V == *UI)
      continue;
    else if (Instruction *I = dyn_cast<Instruction>(*UI))
      if (DT_->dominates(BB, I->getParent()))
        return true;
  return false;
}

// replaceUsesOfWithAfter
void Redefinition::replaceUsesOfWithAfter(Value *V, Value *R, BasicBlock *BB) {
  RDEF_DEBUG(dbgs() << "Redefinition: replaceUsesOfWithAfter: " << *V << " ==> "
                    << *R << " after " << BB->getName() << "\n");

  set<Instruction*> Replace;
  for (auto UI = V->use_begin(), UE = V->use_end(); UI != UE; ++UI)
    if (Instruction *I = dyn_cast<Instruction>(*UI)) {
      // If the instruction's parent dominates BB, mark the instruction to
      // be replaced.
      if (I != R && DT_->dominates(BB, I->getParent())) {
        Replace.insert(I);
      }
      // If parent does not dominate BB, check if the use is a phi and replace
      // the incoming value.
      else if (PHINode *Phi = dyn_cast<PHINode>(*UI))
        for (unsigned Idx = 0; Idx < Phi->getNumIncomingValues(); ++Idx)
          if (Phi->getIncomingValue(Idx) == V &&
              DT_->dominates(BB, Phi->getIncomingBlock(Idx)))
            Phi->setIncomingValue(Idx, R);
    }

  // Replace V with R on all marked instructions.
  for (auto& I : Replace)
    I->replaceUsesOfWith(V, R);
}

