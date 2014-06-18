/*
 * InputDeps.h
 *
 *  Created on: 05/04/2013
 *      Author: raphael
 */

#ifndef VALUECOUNTER_H_
#define VALUECOUNTER_H_

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

        class ValueCounter : public ModulePass {
        public:
			static char ID; // Pass identification, replacement for typeid.
			ValueCounter() : ModulePass(ID){}

			void getAnalysisUsage(AnalysisUsage &AU) const;
			bool runOnModule(Module& M);
        };
}

#endif /* VALUECOUNTER_H_ */
