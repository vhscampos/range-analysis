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
#include "llvm/Analysis/DebugInfo.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/raw_ostream.h"
#include <vector>
#include <list>
#include <map>
#include <stdio.h>

// Use range analysis to avoid unnecessary overflow tests
//#define RANGEANALYSIS

#ifdef RANGEANALYSIS
#include "../RangeAnalysis/RangeAnalysis.h"
#else
#define DEBUG_TYPE "OverflowDetect"
#endif

using namespace llvm;

namespace {

	struct OverflowDetect : public ModulePass {
		static char ID;
		OverflowDetect() : ModulePass(ID) {};

        void printValueInfo(const Value *V);
		void MarkAsNotOriginal(Instruction& inst);
		void InsertGlobalDeclarations();
		BasicBlock* NewOverflowOccurrenceBlock(Instruction* I, BasicBlock* NextBlock);

		void insertInstrumentation(Instruction* I);
		Constant* getSourceFile(Instruction* I);
		Constant* getLineNumber(Instruction* I);

        void PrintInstructionIdentifier(std::string M, std::string F, const Value *V);

        bool IsNotOriginal(Instruction& inst);
        static bool isValidInst(Instruction *I);
        virtual bool runOnModule(Module &M);

        Instruction* GetNextInstruction(Instruction& i);
        
        // Range Analysis stuff
        #ifdef RANGEANALYSIS
        APInt Min, Max;
        
        bool isLimited(const Range &range) {
        	return range.isRegular() && range.getLower().ne(Min) && range.getUpper().ne(Max);
        }
        #endif
        
		virtual void getAnalysisUsage(AnalysisUsage &AU) const {

			//This pass DOES NOT preserve all. It changes the CFG and includes new functions and variables to the source.
			//AU.setPreservesAll();

			#ifdef RANGEANALYIS
			AU.addRequired<InterProceduralRA<Cousot> >();
			#endif
		}

        Module* module;
        llvm::LLVMContext* context;
        Constant* constZero;

        Value* GVstderr, *FPrintF, *messagePtr;



        std::map<std::string,Constant*> SourceFiles;
	};
}

char OverflowDetect::ID = 0;

STATISTIC(NrSignedInsts, "Number of signed instructions instrumented");
STATISTIC(NrUnsignedInsts, "Number of unsigned instructions instrumented");
STATISTIC(NrInstsNotInstrumented, "Number of valid instructions not instrumented");

static RegisterPass<OverflowDetect> X("overflow-detect", "OverflowDetect Instrumentation Pass");


Constant* OverflowDetect::getSourceFile(Instruction* I){

	StringRef File;
	if (MDNode *N = I->getMetadata("dbg")) {
	    DILocation Loc(N);
	    File = Loc.getFilename();
	} else {
		File = "Unknown Source File";
	}

	if (SourceFiles.count(File)) {
		return SourceFiles.find(File)->second;
	} else {

		//Create a global variable with the File string
		Constant* stringConstant = llvm::ConstantArray::get(*context, File, true);
		GlobalVariable* sourceFileStr = new GlobalVariable(*module, stringConstant->getType(), true,
		                                                llvm::GlobalValue::InternalLinkage,
		                                                stringConstant, "SourceFile");

		constZero = ConstantInt::get(Type::getInt32Ty(*context), 0);

		//Get the int8ptr to our message
		Constant* constArray = ConstantExpr::getInBoundsGetElementPtr(sourceFileStr, constZero);
		Constant* sourceFilePtr = ConstantExpr::getBitCast(constArray, PointerType::getUnqual(Type::getInt8Ty(*context)));

		SourceFiles[File] = sourceFilePtr;

		return sourceFilePtr;
	}
}

Constant* OverflowDetect::getLineNumber(Instruction* I){

	if (MDNode *N = I->getMetadata("dbg")) {
	    DILocation Loc(N);
	    unsigned Line = Loc.getLineNumber();
	    return ConstantInt::get(Type::getInt32Ty(*context), Line);
	}	else {
		return ConstantInt::get(Type::getInt32Ty(*context), 0);
	}

}


/*
 * Creates a Basic Block that will be executed when an overflow occurs.
 * It receives an argument that tells what is the next basic block to be executed.
 */
BasicBlock* OverflowDetect::NewOverflowOccurrenceBlock(Instruction* I, BasicBlock* NextBlock){

	Constant* SourceFile = getSourceFile(I);
	Constant* LineNumber = getLineNumber(I);

	BasicBlock* result = BasicBlock::Create(*context, "", I->getParent()->getParent(), NextBlock);
	BranchInst* branch = BranchInst::Create(NextBlock, result);

	Value* vStderr = new LoadInst(GVstderr, "loadstderr", branch);

	std::vector<Value*> args;
	args.push_back(vStderr);
	args.push_back(messagePtr);
	args.push_back(SourceFile);
	args.push_back(LineNumber);
	CallInst::Create(FPrintF, args, "", branch);

	return result;
}

/*
 * Inserts global variable and function declarations, needed during the instrumentation.
 */
void OverflowDetect::InsertGlobalDeclarations(){

	//Create a global variable with the fprintf Message
	Constant* stringConstant = llvm::ConstantArray::get(*context, "Overflow occurred in %s, line %d.\n", true);
	GlobalVariable* messageStr = new GlobalVariable(*module, stringConstant->getType(), true,
	                                                llvm::GlobalValue::InternalLinkage,
	                                                stringConstant, "OverflowMessage");

	//Get the int8ptr to our message
    constZero = ConstantInt::get(Type::getInt32Ty(*context), 0);
	Constant* constArray = ConstantExpr::getInBoundsGetElementPtr(messageStr, constZero);
	messagePtr = ConstantExpr::getBitCast(constArray, PointerType::getUnqual(Type::getInt8Ty(*context)));

	Type* IO_FILE_PTR_ty;

	//Insert the declaration of the stderr global external variable
	GVstderr = module->getGlobalVariable("stderr");

	if (GVstderr == NULL) {

		//Create the types to get the stderr
		StructType* IO_FILE_ty = StructType::create(*context, "struct._IO_FILE");
		IO_FILE_PTR_ty = PointerType::getUnqual(IO_FILE_ty);

		StructType* IO_marker_ty = StructType::create(*context, "struct._IO_marker");
		PointerType* IO_marker_ptr_ty = PointerType::getUnqual(IO_marker_ty);

		std::vector<Type*> Elements;
		Elements.push_back(Type::getInt32Ty(*context));
		Elements.push_back(Type::getInt8PtrTy(*context));
		Elements.push_back(Type::getInt8PtrTy(*context));
		Elements.push_back(Type::getInt8PtrTy(*context));
		Elements.push_back(Type::getInt8PtrTy(*context));
		Elements.push_back(Type::getInt8PtrTy(*context));
		Elements.push_back(Type::getInt8PtrTy(*context));
		Elements.push_back(Type::getInt8PtrTy(*context));
		Elements.push_back(Type::getInt8PtrTy(*context));
		Elements.push_back(Type::getInt8PtrTy(*context));
		Elements.push_back(Type::getInt8PtrTy(*context));
		Elements.push_back(Type::getInt8PtrTy(*context));
		Elements.push_back(IO_marker_ptr_ty);
		Elements.push_back(IO_FILE_PTR_ty);
		Elements.push_back(Type::getInt32Ty(*context));
		Elements.push_back(Type::getInt32Ty(*context));
		Elements.push_back(Type::getInt32Ty(*context));
		Elements.push_back(Type::getInt16Ty(*context));
		Elements.push_back(Type::getInt8Ty(*context));
		Elements.push_back(ArrayType::get(Type::getInt8Ty(*context), 1));
		Elements.push_back(Type::getInt8PtrTy(*context));
		Elements.push_back(Type::getInt64Ty(*context));
		Elements.push_back(Type::getInt8PtrTy(*context));
		Elements.push_back(Type::getInt8PtrTy(*context));
		Elements.push_back(Type::getInt8PtrTy(*context));
		Elements.push_back(Type::getInt8PtrTy(*context));
		Elements.push_back(Type::getInt32Ty(*context));
		Elements.push_back(Type::getInt32Ty(*context));
		Elements.push_back(ArrayType::get(Type::getInt8Ty(*context), 40));
		IO_FILE_ty->setBody(Elements, false);

		std::vector<Type*> Elements2;
		Elements2.push_back(IO_marker_ptr_ty);
		Elements2.push_back(IO_FILE_PTR_ty);
		Elements2.push_back(Type::getInt32Ty(*context));;
		IO_marker_ty->setBody(Elements2, false);

		GVstderr = new GlobalVariable(*module,IO_FILE_PTR_ty,false,GlobalValue::ExternalLinkage, NULL, "stderr");
	} else {

        //Get the type of the stderr
		IO_FILE_PTR_ty = GVstderr->getType();

		//Get the type of the dereference of stderr
		IO_FILE_PTR_ty = IO_FILE_PTR_ty->getContainedType(0);
	}


	//Create list of our fprintf argument types
    std::vector<Type*> Params;
    Params.push_back(IO_FILE_PTR_ty);										//stderr
    Params.push_back(PointerType::getUnqual(Type::getInt8Ty(*context)));	//message
    Params.push_back(PointerType::getUnqual(Type::getInt8Ty(*context)));	//file name
    Params.push_back(Type::getInt32Ty(*context));							//line number

    // Get the fprintf() function (takes an IO_FILE* and an i8* followed by variadic parameters)
    FPrintF = module->getOrInsertFunction("fprintf",FunctionType::get(Type::getVoidTy(*context), Params, true));

}

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
	SourceFiles.clear();
	
	#ifdef RANGEANALYSIS
	InterProceduralRA<Cousot> &ra = getAnalysis<InterProceduralRA<Cousot> >();
	
	this->Min = ra.getMin();
	this->Max = ra.getMax();
	#endif

	NrSignedInsts = 0;
	NrUnsignedInsts = 0;
	NrInstsNotInstrumented = 0;

	//Insert the global declarations (fPrintf, stderr, etc...)
	InsertGlobalDeclarations();

	// Iterate through functions
	for (Module::iterator Fit = M.begin(), Fend = M.end(); Fit != Fend; ++Fit) {
		
		// If the function is empty, do not insert the instrumentation
		// Empty functions include externally linked ones (i.e. abort, printf, scanf, ...)
		if (Fit->begin() == Fit->end())
			continue;

		// Iterate through basic blocks		
		for (Function::iterator BBit = Fit->begin(), BBend = Fit->end(); BBit != BBend; ++BBit) {

			// Iterate through instructions
			for (BasicBlock::iterator Iit = BBit->begin(); Iit != BBit->end(); ++Iit) {
				
				Instruction* I = dyn_cast<Instruction>(Iit);

				if (isValidInst(I) && !IsNotOriginal(*I)){
					#ifdef RANGEANALYSIS
					Range r = ra.getRange(I);
					if (!isLimited(r)) {
						insertInstrumentation(I);
					}
					#else
					insertInstrumentation(I);
					#endif
				}					
			}
		}
	}
    
    // Returns true if the pass make any change to the program
    return (NrSignedInsts + NrUnsignedInsts > 0);
}


void OverflowDetect::insertInstrumentation(Instruction* I){

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
			newBB = I->getParent()->splitBasicBlock(nextIt);

			//This new BB is the BB that contains the overflow check
			newBB2 = BasicBlock::Create(*context, "", I->getParent()->getParent(), newBB);

			// Remove the unconditional branch created by splitBasicBlock, and insert a conditional
			// branch that correctly connects to newBB and newBB2
			branch = cast<BranchInst>(I->getParent()->getTerminator());
			branch->eraseFromParent();

			BranchInst::Create(newBB2, newBB, canCauseOverflow, I->getParent());

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

			BranchInst::Create(NewOverflowOccurrenceBlock(I, newBB), newBB, hasOverflow, newBB2);

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
		newBB = I->getParent()->splitBasicBlock(nextIt);

		// Remove the unconditional branch created by splitBasicBlock, and insert a conditional
		// branch that correctly connects to newBB and assertfail
		branch = cast<BranchInst>(I->getParent()->getTerminator());
		branch->eraseFromParent();

		branch = BranchInst::Create(NewOverflowOccurrenceBlock(I, newBB), newBB, hasOverflow, I->getParent());

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
