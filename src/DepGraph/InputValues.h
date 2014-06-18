/*
 * InputDeps.h
 *
 *  Created on: 05/04/2013
 *      Author: raphael
 */

#ifndef INPUTVALUES_H_
#define INPUTVALUES_H_

#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/CFG.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/raw_ostream.h"
#include <set>
#include <string>

using namespace std;

namespace llvm {


        class InputValues : public ModulePass {
        private:
			llvm::Module* module;

			std::set<Function*> whiteList;
			std::set<Value*> inputValues;

            void initializeWhiteList();
            void insertInWhiteList(Function* F);
            void insertInInputDepValues(Value* V);

            bool isMarkedCallInst(CallInst* CI);

            void collectMainArguments();
        public:
			static char ID; // Pass identification, replacement for typeid.
			InputValues() : ModulePass(ID), module(NULL){}

			void getAnalysisUsage(AnalysisUsage &AU) const;
			bool runOnModule(Module& M);

			bool isInputDependent(Value* V);
			std::set<Value*> getInputDepValues();

        };

}



#endif /* INPUTVALUES_H_ */
