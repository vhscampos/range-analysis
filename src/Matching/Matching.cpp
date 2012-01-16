#define DEBUG_TYPE "matching"

#include "llvm/Module.h"
#include "llvm/Support/CallSite.h"
#include "../RangeAnalysis/RangeAnalysis.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Constants.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/raw_ostream.h"

STATISTIC(numVars, "Number of variables");
STATISTIC(numOps, "Number of operations");

using namespace llvm;

extern unsigned MAX_BIT_INT;
extern APInt Min;
extern APInt Max;

namespace {

	/// Matching
	///
	struct MTCH: public ModulePass {
		static char ID; // Pass identification, replacement for typeid
		MTCH() :
			ModulePass(ID) {
		}

		bool runOnModule(Module &M);
	private:
		void MatchParametersAndReturnValues(Function &F, ConstraintGraph &G);
		unsigned getMaxBitWidth(const Function& F);
	};

	bool MTCH::runOnModule(Module &M) {
	
		DenseMap<const Value*, VarNode*> VarNodes;
		SmallPtrSet<BasicOp*, 64> GenOprs;
		DenseMap<const Value*, SmallPtrSet<BasicOp*, 8> > UseMap;
		//DenseMap<const Value*, SmallPtrSet<BasicOp*, 8> > SymbMap;
		DenseMap<const Value*, ValueBranchMap> ValuesBranchMap;
		
		// Constraint Graph
		//ConstraintGraph G(&VarNodes, &GenOprs, &UseMap, &SymbMap, &ValuesBranchMap);
		ConstraintGraph G(&VarNodes, &GenOprs, &UseMap, &ValuesBranchMap);

		// Search through the functions for the max int bitwidth
		for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I) {
			if (!I->isDeclaration()) {
				unsigned bitwidth = getMaxBitWidth(*I);

				if (bitwidth > MAX_BIT_INT) {
					MAX_BIT_INT = bitwidth;
				}
			}
		}
		
		// Updates the Min and Max values.
		Min = APInt::getSignedMinValue(MAX_BIT_INT);
		Max = APInt::getSignedMaxValue(MAX_BIT_INT);
		
		errs() << MAX_BIT_INT << "\n";


		// Build the Constraint Graph by running on each function
		for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I) {
			// If the function is only a declaration, or if it has variable number of arguments, do not match
			if (I->isDeclaration() || I->isVarArg())
				continue;

			G.buildGraph(*I);

			MatchParametersAndReturnValues(*I, G);
		}
		
		//G.init();
		G.findIntervals();




		// TODO: Fazer o código para imprimir o CG
		
		Function &F = *(M.begin());

		std::string errors;
		std::string filename = M.getModuleIdentifier();
		filename += ".dot";

		raw_fd_ostream output(filename.c_str(), errors);

		G.print(F, output);

		output.close();
		


		// Collect statistics
		numVars = VarNodes.size();
		numOps = GenOprs.size();



		// FIXME: Não sei se tem que retornar true ou false
		return true;
	}

	unsigned MTCH::getMaxBitWidth(const Function& F) {
		unsigned int InstBitSize = 0, opBitSize = 0, max = 0;

		// Obtains the maximum bit width of the instructions of the function.
		for (const_inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
			InstBitSize = I->getType()->getPrimitiveSizeInBits();
			if (I->getType()->isIntegerTy() && InstBitSize > max) {
				max = InstBitSize;
			}

			// Obtains the maximum bit width of the operands of the instruction.
			User::const_op_iterator bgn = I->op_begin(), end = I->op_end();
			for (; bgn != end; ++bgn) {
				opBitSize = (*bgn)->getType()->getPrimitiveSizeInBits();
				if ((*bgn)->getType()->isIntegerTy() && opBitSize > max) {
					max = opBitSize;
				}
			}
		}

		return max;
	}











	void MTCH::MatchParametersAndReturnValues(Function &F, ConstraintGraph &G) {
		// Data structure which contains the matches between formal and real parameters
		// First: formal parameter
		// Second: real parameter
		SmallVector<std::pair<Value*, Value*>, 8> Parameters(F.arg_size());
	
		// Fetch the function arguments (formal parameters) into the data structure
		Function::arg_iterator argptr;
		Function::arg_iterator e;
		unsigned i;

		for (i = 0, argptr = F.arg_begin(), e = F.arg_end(); argptr != e; ++i, ++argptr)
			Parameters[i].first = argptr;

		// Check if the function returns a supported value type. If not, no return value matching is done
		bool noReturn = F.getReturnType()->isVoidTy();

		// Creates the data structure which receives the return values of the function, if there is any
		SmallPtrSet<Value*, 32> ReturnValues;

		if (!noReturn) {
			// Iterate over the basic blocks to fetch all possible return values
			for (Function::iterator bb = F.begin(), bbend = F.end(); bb != bbend; ++bb) {
				// Get the terminator instruction of the basic block and check if it's
				// a return instruction: if it's not, continue to next basic block
				Instruction *terminator = bb->getTerminator();

				ReturnInst *RI = dyn_cast<ReturnInst>(terminator);

				if (!RI)
					continue;

				// Get the return value and insert in the data structure
				ReturnValues.insert(RI->getReturnValue());
			}
		}
	
		// For each use of F, get the real parameters and the caller instruction to do the matching
		for (Value::use_iterator UI = F.use_begin(), E = F.use_end(); UI != E; ++UI) {
			User *U = *UI;

			// Ignore blockaddress uses
			if (isa<BlockAddress>(U))
				continue;

			// Used by a non-instruction, or not the callee of a function, do not
			// match.
			if (!isa<CallInst>(U) && !isa<InvokeInst> (U))
				continue;

			Instruction *caller = cast<Instruction>(U);

			CallSite CS(caller);
			if (!CS.isCallee(UI))
				continue;

			// Iterate over the real parameters and put them in the data structure
			CallSite::arg_iterator AI;
			CallSite::arg_iterator EI;

			for (i = 0, AI = CS.arg_begin(), EI = CS.arg_end(); AI != EI; ++i, ++AI)
				Parameters[i].second = *AI;

			
			// // Do the interprocedural construction of CG
			VarNode* to;
			VarNode* from;
			
			// Match formal and real parameters
			for (i = 0; i < Parameters.size(); ++i) {
				// Add formal parameter to the CG
				to = G.addVarNode(Parameters[i].first);
			
				// Add real parameter to the CG
				from = G.addVarNode(Parameters[i].second);
				
				// Connect nodes
				G.addUnaryOp(to, from);
			}
			
			// Match return values
			if (!noReturn) {
				// Add caller instruction to the CG (it receives the return value)
				to = G.addVarNode(caller);
			
				// For each return value, create a node and connect with an edge
				for (SmallPtrSetIterator<Value*> ri = ReturnValues.begin(), re = ReturnValues.end(); ri != re; ++ri) {
				
					
					// Add VarNode to the CG
					from = G.addVarNode(*ri);
					
					/// Connect nodes
					G.addUnaryOp(to, from);
					
				}
			}
			
	

			// Real parameters are cleaned before moving to the next use (for safety's sake)
			for (i = 0; i < Parameters.size(); ++i)
				Parameters[i].second = NULL;
		}
	}
	
	char MTCH::ID = 0;
	static RegisterPass<MTCH> Y("matching", "Matching Pass");
}
