/*
 * RAInstrumentation.h
 *
 *  Created on: 22/03/2012
 *      Author: raphael
 */

#ifndef RAINSTRUMENTATION_H_
#define RAINSTRUMENTATION_H_

#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/raw_ostream.h"
#include <vector>
#include <list>
#include <stdio.h>
#include "RAInstrumentation.h"

#define BIT_WIDTH 32

using namespace llvm;


namespace llvm {

	struct RAInstrumentation : public ModulePass {
		static char ID;
		RAInstrumentation() : ModulePass(ID) {};

        void printValueInfo(const Value *V);
		void MarkAsNotOriginal(Instruction& inst);
        void PrintInstructionIdentifier(std::string M, std::string F, const Value *V);

        bool IsNotOriginal(Instruction& inst);
        static bool isValidInst(Instruction *I);
        virtual bool runOnModule(Module &M);

        Function& GetSetCurrentMinMaxFunction();
        Function& GetPrintHashFunction();
        Instruction* GetNextInstruction(Instruction& i);
        Constant* strToLLVMConstant(std::string s);

        void InstrumentMainFunction(Function* F, std::string mIdentifier);

        Module* module;
        LLVMContext* context;

	};
}

char RAInstrumentation::ID = 0;





#endif /* RAINSTRUMENTATION_H_ */
