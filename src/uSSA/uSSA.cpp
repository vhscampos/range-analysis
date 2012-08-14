#define DEBUG_TYPE "uSSA"

#include "uSSA.h"
#include "llvm/Support/CommandLine.h"

using namespace llvm;

static const std::string newdefstr = "_newdef";

static cl::opt<bool, false>
TruncInstrumentation("insert-trunc-checks", cl::desc("Insert overflow checks in the trunc instructions."), cl::NotHidden);

bool getTruncInstrumentation(){
	return TruncInstrumentation;
}

void uSSA::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.addRequiredTransitive<DominanceFrontier>();
	AU.addRequiredTransitive<DominatorTree>();
	
	// This pass modifies the program, but not the CFG
	AU.setPreservesCFG();
}

bool uSSA::runOnFunction(Function &F) {
	DT_ = &getAnalysis<DominatorTree>();
	DF_ = &getAnalysis<DominanceFrontier>();
	
	// Iterate over all Basic Blocks of the Function, calling the function that creates sigma functions, if needed
	for (Function::iterator Fit = F.begin(), Fend = F.end(); Fit != Fend; ++Fit) {
		createNewDefs(Fit);
	}
	
	return false;
}

void uSSA::createNewDefs(BasicBlock *BB)
{
	for (BasicBlock::iterator BBit = BB->begin(), BBend = BB->end(); BBit != BBend; ++BBit) {
		Instruction *I = BBit;
		Value *op = NULL;
		ConstantInt *ci = NULL;
		
		BasicBlock::iterator next = BBit;
		++next;
		
		switch (I->getOpcode()) {
			case Instruction::Sub:
			case Instruction::Mul:
			case Instruction::Add:
				// Check if first operand is non-constant AND second operand is a constant
				op = I->getOperand(0);
				ci = dyn_cast<ConstantInt>(I->getOperand(1));
				
				if (!isa<ConstantInt>(op) && ci && (ci->getValue() != 0)) {
					std::string newname = op->getName().str() + newdefstr;
					
					BinaryOperator *newdef = BinaryOperator::Create(Instruction::Add, op, ConstantInt::get(op->getType(), 0), Twine(newname), next);
					
					renameNewDefs(newdef);
					
					// Skip the instruction that has just been created
					++BBit;
				}
				
				break;
			
			case Instruction::Trunc:
				
				if (TruncInstrumentation){
					// Here we don't have a constant, only an interval due to the truncation number of bits
					// Check if first operand is non-constant
					op = I->getOperand(0);
					
					if (!isa<ConstantInt>(op)) {
						std::string newname = op->getName().str() + newdefstr;

						BinaryOperator *newdef = BinaryOperator::Create(Instruction::Add, op, ConstantInt::get(op->getType(), 0), Twine(newname), next);

						renameNewDefs(newdef);

						// Skip the instruction that has just been created
						++BBit;
					}
				}
				break;
		}
	}
}

void uSSA::renameNewDefs(Instruction *newdef)
{
	// This vector of Instruction* points to the uses of V.
	// This auxiliary vector of pointers is used because the use_iterators are invalidated when we do the renaming
	Value *V = newdef->getOperand(0);
	SmallVector<Instruction*, 25> usepointers;
	unsigned i = 0, n = V->getNumUses();
	usepointers.resize(n);
	BasicBlock *BB = newdef->getParent();
	
	for (Value::use_iterator uit = V->use_begin(), uend = V->use_end(); uit != uend; ++uit, ++i)
		usepointers[i] = dyn_cast<Instruction>(*uit);
	
	for (i = 0; i < n; ++i) {
		if (usepointers[i] ==  NULL) {
			continue;
		}
		if (usepointers[i] == newdef) {
			continue;
		}
		if (isa<GetElementPtrInst>(usepointers[i])) {
			continue;
		}
		
		BasicBlock *BB_user = usepointers[i]->getParent();
		BasicBlock::iterator newdefit(newdef);
		BasicBlock::iterator useit(usepointers[i]);
		
		// Check if the use is in the dominator tree of newdef(V)
		if (DT_->dominates(BB, BB_user)) {
			// If in the same basicblock, only rename if the use is posterior to the newdef
			if (BB_user == BB) {
				int dist1 = std::distance(BB->begin(), useit);
				int dist2 = std::distance(BB->begin(), newdefit);
				int offset = dist1 - dist2;
				
				if (offset > 0) {
					usepointers[i]->replaceUsesOfWith(V, newdef);
				}
			}
			else {
				usepointers[i]->replaceUsesOfWith(V, newdef);
			}
		}
	}
}

char uSSA::ID = 0;
static RegisterPass<uSSA> X("ussa", "u-SSA");
