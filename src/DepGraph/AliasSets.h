#ifndef __ALIAS_SETS_H__
#define __ALIAS_SETS_H__

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/AliasSetTracker.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/ADT/ilist.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/PassAnalysisSupport.h"
#include "PADriver.h"
#include "llvm/Support/Debug.h"


#include <cassert>
#include<set>
#include<map>
#include<queue>

using namespace std;

namespace llvm {
	class AliasSets: public ModulePass {


private:
		llvm::DenseMap<int, int> disjointSet; // maps integers to the disjoint sets that contains the integer
		llvm::DenseMap<int, std::set<int> > disjointSets; // table of disjoint sets

		llvm::DenseMap<const Value*, int> valueDisjointSet; // maps integers to the disjoint sets that contains the integer
		llvm::DenseMap<int, std::set<Value*> > valueDisjointSets; // table of disjoint sets

		bool runOnModule(Module &M);
		void printSets();

	public:
		static char ID;
		AliasSets() :
				ModulePass(ID) {
		}
		;

		void getAnalysisUsage(AnalysisUsage &AU) const;
		llvm::DenseMap<int, std::set<Value*> > getValueSets();
		llvm::DenseMap<int, std::set<int> > getMemSets();
		int getValueSetKey(const Value* v);
		int getMapSetKey(int m);
	};
}

#endif
