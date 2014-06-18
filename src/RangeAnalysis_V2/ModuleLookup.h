/*
 * ModuleLookup.h
 *
 *  Created on: Jun 15, 2014
 *      Author: raphael
 */

#ifndef MODULELOOKUP_H_
#define MODULELOOKUP_H_

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"

#include <map>
#include <string>

using namespace std;

namespace llvm {

typedef std::map<std::string, Value*> ValueLookup;


class ModuleLookup: public ModulePass {

	Module* module;
	std::map<Function*, ValueLookup> FunctionLookup;

public:
	static char ID;

	ModuleLookup(): ModulePass(ID), module(NULL) {}

	virtual ~ModuleLookup() {}

    void getAnalysisUsage(AnalysisUsage &AU) const {
        AU.setPreservesAll();
    }

    bool runOnModule(Module &M);

    Value* getValueByName(std::string FunctionName, std::string ValueName);
};

} /* namespace llvm */

#endif /* MODULELOOKUP_H_ */
