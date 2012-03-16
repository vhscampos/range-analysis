//===- RAInstrumentation.cpp - Example code from "Writing an LLVM Pass" ---===//
//
// This file implements the LLVM "Range Analysis Instrumentation" pass 
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "hello"

#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Function.h"
#include "llvm/InstrTypes.h"
#include "llvm/Instructions.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/raw_ostream.h"
#include <vector>
#include <list>
#include <stdio.h>

STATISTIC(numInstructions, "Number of instructions instrumented");

using namespace llvm;

const unsigned BIT_WIDTH = 32;

namespace {

	struct RAInstrumentation : public ModulePass {
		static char ID; 
		RAInstrumentation() : ModulePass(ID) {};
		
        void printValueInfo(const Value *V);
		void MarkAsNotOriginal(Instruction& inst);
        void PrintInstructionIdentifier(std::string M, std::string F, const Value *V);
        
        bool IsNotOriginal(Instruction& inst);       
        bool isValidInst(Instruction *I);
        virtual bool runOnModule(Module &M);
        
        Function& GetSetCurrentMinMaxFunction();
        Function& GetPrintHashFunction();
        Instruction* GetNextInstruction(Instruction& i);
        
        void InstrumentMainFunction(Function* F);
        
        Module* module;
        LLVMContext* context;
		FILE* file;

	};
}

char RAInstrumentation::ID = 0;
static RegisterPass<RAInstrumentation> X("ra-instrumentation", "Range Analysis Instrumentation Pass");


bool RAInstrumentation::isValidInst(Instruction *I)
{
	// Only i32 instructions are valid
	// Exceptions: invoke instructions
	bool conditions = !I->getType()->isIntegerTy(BIT_WIDTH) || isa<InvokeInst>(I);
	
	return !conditions;
}


void RAInstrumentation::PrintInstructionIdentifier(std::string M, std::string F, const Value *V){

    
	fprintf(file, "%s.%s.%s %d\n", M.c_str(), F.c_str(), V->getName().str().c_str(), (int)V);

}


void RAInstrumentation::InstrumentMainFunction(Function* F) {

    //Put a call to printHash function before every return instruction.
    
    
	// Iterate through basic blocks
	for (Function::iterator BBit = F->begin(); BBit != F->end(); ++BBit) {

    	// Iterate through instructions
    	for (BasicBlock::iterator Iit = BBit->begin(); Iit != BBit->end(); ++Iit) {     		
            if (isa<ReturnInst>(Iit)){
                Function& printHash = GetPrintHashFunction();                         
                CallInst* callPrintHash = CallInst::Create(&printHash, "", Iit);  
                MarkAsNotOriginal(*callPrintHash);
            }
        }
    };
}


bool RAInstrumentation::runOnModule(Module &M) {

	this->module = &M;
	this->context = &M.getContext();
    std::string ErrorInfo;

    file = fopen("RAHashNames.txt","w");
    if (file==NULL) {
        errs() << "Error opening file RAHashNames.txt. Range Analysis Instrumentation Pass aborted.\n";
        return false;
    }
    
    // No main, no instrumentation!
    Function *Main = M.getFunction("main");
    
    // Using fortran? ... this kind of works
    if (!Main)
        Main = M.getFunction("MAIN__");
    
    if (!Main) {
        errs() << "WARNING: cannot insert instrumentation into a module"
               << " with no main function!\n";
        return false;
    }  
    
    InstrumentMainFunction(Main);
    
	// Iterate through functions
	for (Module::iterator Fit = M.begin(), Fend = M.end(); Fit != Fend; ++Fit) {
		
		// If the function is empty, do not insert the instrumentation
		// Empty functions include externally linked ones (i.e. abort, printf, scanf, ...)
		if (Fit->begin() == Fit->end())
			continue;
        
		// Iterate through basic blocks		
		for (Function::iterator BBit = Fit->begin(), BBend = Fit->end(); BBit != BBend; ++BBit) {

            std::list<Value*> args; 

			// Iterate through instructions
			for (BasicBlock::iterator Iit = BBit->begin(); Iit != BBit->end(); ++Iit) {
				
				if (isValidInst(Iit)&& ! IsNotOriginal(*Iit)){

                    //Add the identifier of the instruction to the Hash Dictionary
                    PrintInstructionIdentifier(M.getModuleIdentifier(),Fit->getName().str(),Iit);
                
                    //Get the pointer to the instruction. Note that the same number was saved in the Hash Dictionary and it will be used into the compiled program. 
                    Value* vInstruction = cast<Value>(Iit);
                    
                    Constant* constInt = ConstantInt::get(Type::getInt32Ty(*context), (uint64_t)vInstruction);
                    Value* constPtr = ConstantExpr::getIntToPtr(constInt, PointerType::getUnqual(Type::getInt8Ty(*context))); 
                
                    //Insert the arguments into a queue. The queue will be consumed when the next instruction is not a PHI function. 
                    args.push_back(constPtr);
                    args.push_back(Iit);
                    
                    Instruction* nextInstruction = GetNextInstruction(*Iit);  
                    
                    if (!isa<PHINode>(nextInstruction)) 
                    {
						Function& setCurrentMinMax = GetSetCurrentMinMaxFunction();
                        
                        while (!args.empty()) 
                        {                                
                             
                            
                            std::vector<Value*> setCurrentMinMaxArgs;   
                            setCurrentMinMaxArgs.push_back(args.front());
                            args.pop_front();
                            setCurrentMinMaxArgs.push_back(args.front());  
                            args.pop_front();
                                                  
                            CallInst* callSetCurrentMinMax = CallInst::Create(&setCurrentMinMax, setCurrentMinMaxArgs, "", nextInstruction);  
                            MarkAsNotOriginal(*callSetCurrentMinMax);
    
                            ++numInstructions;
                        }
                    }
				}					
			}
		}
	}
	
    fclose(file);
    
    // Returns true if the pass make any change to the program
    return (numInstructions > 0);
}

void RAInstrumentation::MarkAsNotOriginal(Instruction& inst)
{
        inst.setMetadata("new-inst", MDNode::get(*context, std::vector<Value*>()));
}
bool RAInstrumentation::IsNotOriginal(Instruction& inst)
{
        return inst.getMetadata("new-inst") != 0;
}


Instruction* RAInstrumentation::GetNextInstruction(Instruction& i)
{
	BasicBlock::iterator it(&i);
	it++;
	return it;
}

// Insert the declaration to the setCurrentMinMax function
Function& RAInstrumentation::GetSetCurrentMinMaxFunction()
{
	static Function* func = 0;
	
	if (func) return *func;
	
	std::vector<Type*> args;
	args.push_back(Type::getInt8PtrTy(*context));
	args.push_back(Type::getInt32Ty(*context));    
	func = Function::Create(FunctionType::get(Type::getVoidTy(*context), args, false), GlobalValue::ExternalLinkage, "setCurrentMinMax", module);
	return *func;
}

// Insert the declaration to the printHash function
Function& RAInstrumentation::GetPrintHashFunction()
{
	static Function* func = 0;
	
	if (func) return *func;
	 
	func = Function::Create(FunctionType::get(Type::getVoidTy(*context), false), GlobalValue::ExternalLinkage, "printHash", module);
	return *func;
}
