//===- OverflowDetect.cpp - Example code from "Writing an LLVM Pass" ---===//
//
// This file implements the LLVM "Range Analysis Instrumentation" pass 
//
//===----------------------------------------------------------------------===//
#include "llvm/Constants.h"
#include "llvm/Operator.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Function.h"
#include "llvm/InstrTypes.h"
#include "llvm/Instructions.h"
#include "llvm/Instruction.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/raw_ostream.h"
#include <vector>
#include <list>
#include <stdio.h>

#include "../RangeAnalysis/RangeAnalysis.h"

#define BIT_WIDTH 32
//#define DEBUG_TYPE "OverflowDetect"

using namespace llvm;

namespace {

	struct OverflowDetect : public ModulePass {
		static char ID;
		OverflowDetect() : ModulePass(ID) {};

        void printValueInfo(const Value *V);
		void MarkAsNotOriginal(Instruction& inst);
		void insertInstrumentation(Instruction* I, BasicBlock* CurrentBB, BasicBlock *assertfail);
        void PrintInstructionIdentifier(std::string M, std::string F, const Value *V);

        bool IsNotOriginal(Instruction& inst);
        static bool isValidInst(Instruction *I);
        virtual bool runOnModule(Module &M);

        Instruction* GetNextInstruction(Instruction& i);
        
        APInt Min, Max;
		
        bool isLimited(const Range &range) {
        	return range.isRegular() && range.getLower().ne(Min) && range.getUpper().ne(Max);
        }
        
		virtual void getAnalysisUsage(AnalysisUsage &AU) const {
			AU.setPreservesAll();
			AU.addRequired<InterProceduralRA<Cousot> >();
		}

        Module* module;
        llvm::LLVMContext* context;
        Constant* constZero;

	};
}

char OverflowDetect::ID = 0;

STATISTIC(NrSignedInsts, "Number of signed instructions instrumented");
STATISTIC(NrUnsignedInsts, "Number of unsigned instructions instrumented");
STATISTIC(NrInstsNotInstrumented, "Number of valid instructions not instrumented");

static RegisterPass<OverflowDetect> X("overflow-detect", "OverflowDetect Instrumentation Pass");

/*
 * 	In order to be valid, an instruction must be of intBITWIDTH type, and its operands as well
 */
bool OverflowDetect::isValidInst(Instruction *I)
{
	OverflowingBinaryOperator *bin = dyn_cast<OverflowingBinaryOperator>(I);

	return bin && bin->getType()->isIntegerTy() && bin->getOperand(0)->getType()->isIntegerTy() && bin->getOperand(1)->getType()->isIntegerTy();
}

bool OverflowDetect::runOnModule(Module &M) {

	this->module = &M;
	this->context = &M.getContext();
	this->constZero = NULL;
	
	InterProceduralRA<Cousot> &ra = getAnalysis<InterProceduralRA<Cousot> >();
	
	this->Min = APInt::getSignedMinValue(ra.getBitWidth());
	this->Max = APInt::getSignedMaxValue(ra.getBitWidth());


	NrSignedInsts = 0;
	NrUnsignedInsts = 0;
	NrInstsNotInstrumented = 0;


	// Get void function type
	FunctionType *AbortFTy = FunctionType::get(Type::getVoidTy(*context), false);

	// Pointer to abort function
	Function *AbortF = NULL;


	/*
	 *	Initialization of 'abort' function
	 */

	// Before declaring the abort function, we need to check whether it is already declared or not
	Module::iterator Fit, Fend;
	bool alreadyDeclared = false;

	for (Fit = M.begin(), Fend = M.end(); Fit != Fend; ++Fit)
		if (Fit->getName().equals("abort")) {
			alreadyDeclared = true;
			AbortF = Fit;
			break;
		}

	if (!alreadyDeclared) {
		AbortF = Function::Create(AbortFTy, GlobalValue::ExternalLinkage, "abort", &M);
	}


	// Iterate through functions
	for (Module::iterator Fit = M.begin(), Fend = M.end(); Fit != Fend; ++Fit) {
		
		// If the function is empty, do not insert the instrumentation
		// Empty functions include externally linked ones (i.e. abort, printf, scanf, ...)
		if (Fit->begin() == Fit->end())
			continue;

		// Create the basic block which the controw flow goes to when the assertion fail
		BasicBlock *assertfail = BasicBlock::Create(*context, "assert fail", dyn_cast<Function>(Fit));

		// Call to abort function
		CallInst *abort = CallInst::Create(AbortF, Twine(), assertfail);

		// Add atributes to the abort call instruction: no return and no unwind
		abort->addAttribute(~0, Attribute::NoReturn);
		abort->addAttribute(~0, Attribute::NoUnwind);

		// Unreachable instruction
		new UnreachableInst(*context, assertfail);

        
		// Iterate through basic blocks		
		for (Function::iterator BBit = Fit->begin(), BBend = Fit->end(); BBit != BBend; ++BBit) {

			// If the basic block is the assertfail, continue (probably assertfail is the last BB in the list, so actually this may end the loop)
			if (cast<BasicBlock>(BBit) == assertfail)
				continue;

			// Iterate through instructions
			for (BasicBlock::iterator Iit = BBit->begin(); Iit != BBit->end(); ++Iit) {
				
				Instruction* I = dyn_cast<Instruction>(Iit);
				
				Range r = ra.getRange(I);

				if (isValidInst(I) && !IsNotOriginal(*I) && !isLimited(r)){
					insertInstrumentation(I, dyn_cast<BasicBlock>(BBit), assertfail);
				}					
			}
		}
	}
    
    // Returns true if the pass make any change to the program
    return (NrSignedInsts + NrUnsignedInsts > 0);
}


void OverflowDetect::insertInstrumentation(Instruction* I, BasicBlock* CurrentBB, BasicBlock *assertfail){

	// Fetch the boundaries of the variable

	OverflowingBinaryOperator *op = dyn_cast<OverflowingBinaryOperator>(I);

	bool isSigned;

	if (op->hasNoUnsignedWrap()){				//nuw flag
		isSigned = false;
		NrUnsignedInsts++;
	} else if (op->hasNoSignedWrap()) {			//nsw flag
		isSigned = true;
		NrSignedInsts++;
	} else {
		NrInstsNotInstrumented++;
		return;
	}

//	constZero = ConstantInt::get(IntegerType::get(*context, op->getType()->getPrimitiveSizeInBits()), 0);
	constZero = ConstantInt::get(op->getType(), 0);

	Value* hasOverflow = NULL;

    BinaryOperator* bin = dyn_cast<BinaryOperator>(I);
    Value* op1 = bin->getOperand(0);
    Value* op2 = bin->getOperand(1);

	// Create comparison instructions, according to the may-overflow instruction.
	// They are inserted just after the instruction I
    Instruction* nextInstruction = GetNextInstruction(*I);
    BasicBlock::iterator nextIt(nextInstruction);


    ICmpInst *positiveOp1;
    ICmpInst *positiveOp2;

    ICmpInst *negativeOp1;

    ICmpInst *lessThanOp1;
    ICmpInst *lessThanOp2;

    ICmpInst *negativeResult;
    ICmpInst *positiveResult;

    Value* canCauseOverflow;

    Value* tmpValue;

    BasicBlock *newBB, *newBB2;
    BranchInst *branch;

	switch(I->getOpcode()){

		case Instruction::Add:

			if (isSigned){
				//How to do an overflow with a signed Add instruction:
				//		add two big positive numbers and get a negative number as result
				positiveOp1 = new ICmpInst(nextInstruction, CmpInst::ICMP_SGT, op1, constZero);
				positiveOp2 = new ICmpInst(nextInstruction, CmpInst::ICMP_SGT, op2, constZero);
				canCauseOverflow = BinaryOperator::Create(Instruction::And, positiveOp1, positiveOp2, "", nextInstruction);

				negativeResult = new ICmpInst(nextInstruction, CmpInst::ICMP_SLT, I, constZero);

				// AND the results
				hasOverflow = BinaryOperator::Create(Instruction::And, canCauseOverflow, negativeResult, "", nextInstruction);
			} else {
				//How to do an overflow with a unsigned Add instruction:
				//		add two numbers and get a result smaller than one of the operands
				lessThanOp1 = new ICmpInst(nextInstruction, CmpInst::ICMP_ULT, dyn_cast<Value>(I), op1);
				lessThanOp2 = new ICmpInst(nextInstruction, CmpInst::ICMP_ULT, dyn_cast<Value>(I), op2);
				hasOverflow = BinaryOperator::Create(Instruction::Or, lessThanOp1, lessThanOp2, "", nextInstruction);
			}
			break;

		case Instruction::Sub:

			if (isSigned) {
				//How to do an overflow with a signed Sub instruction:
				//		subtract a positive number from a negative number and get a positive number as result
				negativeOp1 = new ICmpInst(nextInstruction, CmpInst::ICMP_SLT, op1, constZero);
				positiveOp2 = new ICmpInst(nextInstruction, CmpInst::ICMP_SGT, op2, constZero);
				canCauseOverflow = BinaryOperator::Create(Instruction::And, negativeOp1, positiveOp2, "", nextInstruction);

				positiveResult = new ICmpInst(nextInstruction, CmpInst::ICMP_SLT, I, constZero);

				// AND the results
				hasOverflow = BinaryOperator::Create(Instruction::And, canCauseOverflow, positiveResult, "", nextInstruction);
			} else {

				//How to do an overflow with an unsigned Sub instruction:
				//		subtract a number from a smaller number. It should result a negative number, but Unsigned numbers
				//		don't store negative numbers. There is our overflow.
				hasOverflow = new ICmpInst(nextInstruction, CmpInst::ICMP_ULT, op1, op2);

			}
			break;

		case Instruction::Mul:

			//How to verify if an overflow has just happened :
			//	divide the result by one of the operands of the multiplication.
			//  If the result of the division is not equal the other operand, there is an overflow
			// 	(It can be an expensive test. If it gets too expensive, we can test it in terms of
			//	 the most significant bit of the operators and the most significant bit of the result)

			//First verify if the operand op1 is zero (it would cause a divide-by-zero exception)
			canCauseOverflow = new ICmpInst(nextInstruction, CmpInst::ICMP_NE, op1, constZero);

			// Move all remaining instructions of the basic block to a new one
			// This new BB is where controw flow goes to when the assertion is correct
			newBB = CurrentBB->splitBasicBlock(nextIt);

			newBB2 = BasicBlock::Create(*context, "", CurrentBB->getParent(), newBB);

			// Remove the unconditional branch created by splitBasicBlock, and insert a conditional
			// branch that correctly connects to newBB and assertfail
			branch = cast<BranchInst>(CurrentBB->getTerminator());
			branch->eraseFromParent();

			BranchInst::Create(newBB2, newBB, canCauseOverflow, CurrentBB);

			branch = BranchInst::Create(newBB, newBB2);

			if (isSigned){

				tmpValue = BinaryOperator::Create(Instruction::SDiv, I, op1, "", branch);
				hasOverflow = new ICmpInst(branch, CmpInst::ICMP_NE, tmpValue, op2);

			} else {

				tmpValue = BinaryOperator::Create(Instruction::UDiv, I, op1, "", branch);
				hasOverflow = new ICmpInst(branch, CmpInst::ICMP_NE, tmpValue, op2);

			}

			// Remove the unconditional branch created by splitBasicBlock, and insert a conditional
			// branch that correctly connects to newBB and assertfail
			branch = cast<BranchInst>(newBB2->getTerminator());
			branch->eraseFromParent();

			BranchInst::Create(assertfail, newBB, hasOverflow, newBB2);

			break;

		case Instruction::Shl:

			//The result of a Shift Left must be greater than the first operand.
			//	Case the result is less than the first operand, there is an overflow
			if (isSigned){

				hasOverflow = new ICmpInst(nextInstruction, CmpInst::ICMP_SLT, I, op1);

			} else {

				hasOverflow = new ICmpInst(nextInstruction, CmpInst::ICMP_ULT, I, op1);

			}
			break;

	}


	if (I->getOpcode() != Instruction::Mul) {

		// Move all remaining instructions of the basic block to a new one
		// This new BB is where controw flow goes to when the assertion is correct
		newBB = CurrentBB->splitBasicBlock(nextIt);

		// Remove the unconditional branch created by splitBasicBlock, and insert a conditional
		// branch that correctly connects to newBB and assertfail
		branch = cast<BranchInst>(CurrentBB->getTerminator());
		branch->eraseFromParent();

		branch = BranchInst::Create(assertfail, newBB, hasOverflow, CurrentBB);

	}
}

void OverflowDetect::MarkAsNotOriginal(Instruction& inst)
{
        inst.setMetadata("new-inst", MDNode::get(*context, llvm::ArrayRef<Value*>()));
}
bool OverflowDetect::IsNotOriginal(Instruction& inst)
{
        return inst.getMetadata("new-inst") != 0;
}


Instruction* OverflowDetect::GetNextInstruction(Instruction& i)
{
	BasicBlock::iterator it(&i);
	it++;
	return it;
}
