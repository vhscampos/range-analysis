//===- OverflowDetect.cpp - Example code from "Writing an LLVM Pass" ---===//
//
// This file implements the LLVM "Range Analysis Instrumentation" pass 
//
//===----------------------------------------------------------------------===//
#include "llvm/IR/Type.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/DebugInfo.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"
#include "uSSA/uSSA.h"
#include <vector>
#include <list>
#include <map>
#include <stdio.h>

#include "../RangeAnalysis/RangeAnalysis.h"

using namespace llvm;

typedef enum { OvUnknown, OvCanHappen, OvWillHappen, OvWillNotHappen } OvfPrediction;

static cl::opt<bool, false>
UseRaPrunning("use-ra-prunning", cl::desc("Use range analysis to avoid inserting unnecessary checks."), cl::NotHidden);

static cl::opt<bool, false>
InsertAborts("insert-ovf-aborts", cl::desc("Abort the instrumented program when an overflow occurs."), cl::NotHidden);

static cl::opt<bool, false>
InsertFprintfs("insert-stderr-fprintfs", cl::desc("Insert fprintf calls when overflow occurs."), cl::NotHidden, cl::init(true));


//Declaration of this argument is in uSSA.cpp. Public function is in uSSA.h
bool TruncInstrumentation = getTruncInstrumentation();



namespace {

	struct OverflowDetect : public ModulePass {
		static char ID;
		OverflowDetect() : ModulePass(ID) {};

        void printValueInfo(const Value *V);
		void MarkAsNotOriginal(Instruction& inst);
		void InsertGlobalDeclarations();
		BasicBlock* NewOverflowOccurrenceBlock(Instruction* I, BasicBlock* NextBlock, Value *messagePtr);

		void insertInstrumentation(Instruction* I, BasicBlock* AbortBB, OvfPrediction Pred);
		Constant* getSourceFile(Instruction* I);
		Constant* getLineNumber(Instruction* I);

        void PrintInstructionIdentifier(std::string M, std::string F, const Value *V);

        bool IsNotOriginal(Instruction& inst);
        static bool isValidInst(Instruction *I);
        virtual bool runOnModule(Module &M);

        Instruction* GetNextInstruction(Instruction& i);
        
        // Range Analysis stuff
        APInt Min, Max;
        
        bool isLimited(const Range &range) {
        	return range.isRegular() && range.getLower().ne(Min) && range.getUpper().ne(Max);
        }
        
        OvfPrediction ovfStaticAnalysis(Instruction* I, InterProceduralRA<Cousot> *ra);

		virtual void getAnalysisUsage(AnalysisUsage &AU) const {

			if (UseRaPrunning)
				AU.addRequired<InterProceduralRA<Cousot> >();

		}

        Module* module;
        llvm::LLVMContext* context;
        Constant* constZero;

        Value* GVstderr, *FPrintF, *overflowMessagePtr, *truncErrorMessagePtr, *overflowMessagePtr2, *truncErrorMessagePtr2;

        // Pointer to abort function
        Function *AbortF;

        std::map<std::string,Constant*> SourceFiles;
	};
}

char OverflowDetect::ID = 0;

STATISTIC(NrInsts, "Number of instructions");
STATISTIC(NrPrunnedInsts, "Number of prunned instructions");
STATISTIC(NrSignedInsts, "Number of signed instructions instrumented");
STATISTIC(NrUnsignedInsts, "Number of unsigned instructions instrumented");
STATISTIC(NrOvfStaticallyDetected, "Number of statically detected overflows");
STATISTIC(NrPossibleOvfStaticallyDetected, "Number of possible overflows statically detected");

static RegisterPass<OverflowDetect> X("overflow-detect", "OverflowDetect Instrumentation Pass");

static bool isSignedInst(Value* V){
	Instruction* I = dyn_cast<Instruction>(V);

	if (I == NULL) return false;

	if (dyn_cast<FPToSIInst>(V)) return true;

	switch(I->getOpcode()){

		case Instruction::SDiv:
		case Instruction::SRem:
		case Instruction::AShr:
		case Instruction::SExt:
			return true;
			break;

		case Instruction::Add:
		case Instruction::Sub:
		case Instruction::Mul:
		case Instruction::Shl:
			return dyn_cast<OverflowingBinaryOperator>(I)->hasNoSignedWrap();
			break;

		case Instruction::BitCast:
		case Instruction::Trunc:
			return isSignedInst(I->getOperand(0));
			break;

		default:
			return false;
	}


}

OvfPrediction OverflowDetect::ovfStaticAnalysis(Instruction* I, InterProceduralRA<Cousot> *ra){

	if (!UseRaPrunning) return OvUnknown;

	Range r;
	if (I->getOpcode() == Instruction::Trunc)
		r = ra->getRange(I->getOperand(0));
	else
		r = ra->getRange(I);

	if (!isLimited(r)) {
		return OvUnknown;

	} else {

		unsigned numBits = I->getType()->getPrimitiveSizeInBits();
		
		//Check if the Upper and Lower bounds of the range are sound
		if(r.getUpper().getBitWidth() != r.getLower().getBitWidth()){
			errs() << "WARNING! Upper and lower bounds with different bitwidths.\n";		
			return OvUnknown;
		}
		
		//Check if the range is contained within the type bounds, because it is represented with less bits
		if (r.getUpper().getBitWidth() < numBits || r.getLower().getBitWidth() < numBits) {
			NrPrunnedInsts++;
			return OvWillNotHappen;
		}		

		if (isSignedInst(I)) {
			
			APInt MinValue = APInt::getSignedMinValue(numBits);
			APInt MaxValue = APInt::getSignedMaxValue(numBits);

			//If needed, do the signed extend of everyone to the same bit width
			if (MinValue.getBitWidth() < r.getUpper().getBitWidth()){
				MinValue = MinValue.sext(r.getUpper().getBitWidth());
				MaxValue = MaxValue.sext(r.getUpper().getBitWidth());					
			}
						
			if (MinValue.sgt(r.getUpper()) || MaxValue.slt(r.getLower())) {
				NrOvfStaticallyDetected++;
				return OvWillHappen;
			}
			else if (MinValue.sgt(r.getLower()) || MaxValue.slt(r.getUpper())) {
				NrPossibleOvfStaticallyDetected++;
				return OvCanHappen;
			}
			else {
				NrPrunnedInsts++;
				return OvWillNotHappen;
			}

		} else {
			
			APInt Zero(numBits, 0);
			//FIXME: Our RA doesn't handle unsigned instructions very well.
			// So, I'm using the signed max value to do the analysis. It needs to be fixed.
			APInt MaxValue = APInt::getSignedMaxValue(numBits);

			//If needed, do the unsigned extend of everyone to the same bit width
			if (MaxValue.getBitWidth() < r.getUpper().getBitWidth()){
				Zero = Zero.zext(r.getUpper().getBitWidth());
				MaxValue = MaxValue.zext(r.getUpper().getBitWidth());					
			}

			/* We don't have the precision needed to say that an overflow will happen.
			if (MaxValue.ult(r.getLower())) {
				NrOvfStaticallyDetected++;
				return OvWillHappen;
			}
			else*/ if (MaxValue.ult(r.getUpper())) {
				NrPossibleOvfStaticallyDetected++;
				return OvCanHappen;
			}
			else if (r.getLower().uge(Zero) && r.getUpper().ult(MaxValue)) {
				NrPrunnedInsts++;
				return OvWillNotHappen;
			}
			else
				return OvUnknown;

		}

	}

}


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
		Constant* stringConstant = llvm::ConstantArray::get(*context, File);
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
BasicBlock* OverflowDetect::NewOverflowOccurrenceBlock(Instruction* I, BasicBlock* NextBlock, Value *messagePtr){

	Constant* SourceFile = getSourceFile(I);
	Constant* LineNumber = getLineNumber(I);
	Constant* InstructionIdentifier = ConstantInt::get(Type::getInt32Ty(*context), (int)I);

	BasicBlock* result = BasicBlock::Create(*context, "", I->getParent()->getParent(), NextBlock);
	BranchInst* branch = BranchInst::Create(NextBlock, result);

	if (InsertFprintfs){
		Value* vStderr = new LoadInst(GVstderr, "loadstderr", branch);

		std::vector<Value*> args;
		args.push_back(vStderr);
		args.push_back(messagePtr);
		args.push_back(SourceFile);
		args.push_back(LineNumber);
		args.push_back(InstructionIdentifier);
		CallInst::Create(FPrintF, args, "", branch);
	}

	return result;
}

/*
 * Inserts global variable and function declarations, needed during the instrumentation.
 */
void OverflowDetect::InsertGlobalDeclarations(){

	//Create a global variable with the fprintf Message
	Constant* stringConstant = llvm::ConstantArray::get(*context, "Overflow occurred in %s, line %d. [%d]\n", true);
	GlobalVariable* messageStr = new GlobalVariable(*module, stringConstant->getType(), true,
	                                                llvm::GlobalValue::InternalLinkage,
	                                                stringConstant, "OverflowMessage");

	//Get the int8ptr to our message
    constZero = ConstantInt::get(Type::getInt32Ty(*context), 0);
	Constant* constArray = ConstantExpr::getInBoundsGetElementPtr(messageStr, constZero);
	overflowMessagePtr = ConstantExpr::getBitCast(constArray, PointerType::getUnqual(Type::getInt8Ty(*context)));


	stringConstant = llvm::ConstantArray::get(*context, "Truncation with data loss occurred in %s, line %d. [%d]\n", true);
	messageStr = new GlobalVariable(*module, stringConstant->getType(), true,
	                                                llvm::GlobalValue::InternalLinkage,
	                                                stringConstant, "TruncErrorMessage");

	//Get the int8ptr to our message
	constArray = ConstantExpr::getInBoundsGetElementPtr(messageStr, constZero);
	truncErrorMessagePtr = ConstantExpr::getBitCast(constArray, PointerType::getUnqual(Type::getInt8Ty(*context)));


	//Messages for the overflows statically detected (suspect instructions)
	stringConstant = llvm::ConstantArray::get(*context, "(Suspected) Overflow occurred in %s, line %d. [%d]\n", true);
	messageStr = new GlobalVariable(*module, stringConstant->getType(), true,
	                                                llvm::GlobalValue::InternalLinkage,
	                                                stringConstant, "OverflowMessage");

	//Get the int8ptr to our message
	constArray = ConstantExpr::getInBoundsGetElementPtr(messageStr, constZero);
	overflowMessagePtr2 = ConstantExpr::getBitCast(constArray, PointerType::getUnqual(Type::getInt8Ty(*context)));

	stringConstant = llvm::ConstantArray::get(*context, "(Suspected) Truncation with data loss occurred in %s, line %d. [%d]\n", true);
	messageStr = new GlobalVariable(*module, stringConstant->getType(), true,
	                                                llvm::GlobalValue::InternalLinkage,
	                                                stringConstant, "TruncErrorMessage");

	//Get the int8ptr to our message
	constArray = ConstantExpr::getInBoundsGetElementPtr(messageStr, constZero);
	truncErrorMessagePtr2 = ConstantExpr::getBitCast(constArray, PointerType::getUnqual(Type::getInt8Ty(*context)));


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
    Params.push_back(Type::getInt32Ty(*context));							//Instruction Identifier

    // Get the fprintf() function (takes an IO_FILE* and an i8* followed by variadic parameters)
    FPrintF = module->getOrInsertFunction("fprintf",FunctionType::get(Type::getVoidTy(*context), Params, true));

    if (InsertAborts){
		// Get void function type
		FunctionType *AbortFTy = FunctionType::get(Type::getVoidTy(*context), false);

		/*
		 *      Initialization of 'abort' function
		 */

		// Before declaring the abort function, we need to check whether it is already declared or not
		Module::iterator Fit, Fend;
		bool alreadyDeclared = false;

		for (Fit = module->begin(), Fend =  module->end(); Fit != Fend; ++Fit){
			if (Fit->getName().equals("abort")) {
					alreadyDeclared = true;
					AbortF = Fit;
					break;
			}
		}

		if (!alreadyDeclared) {
				AbortF = Function::Create(AbortFTy, GlobalValue::ExternalLinkage, "abort", module);
		}
    }


}

/*
 * 	In order to be valid, an instruction must be of intBITWIDTH type, and its operands as well
 */
bool OverflowDetect::isValidInst(Instruction *I)
{
	if (dyn_cast<OverflowingBinaryOperator>(I))
		return I->getType()->isIntegerTy() && I->getOperand(0)->getType()->isIntegerTy() && I->getOperand(1)->getType()->isIntegerTy();
	else if (dyn_cast<TruncInst>(I))
		return TruncInstrumentation && I->getType()->isIntegerTy() && I->getOperand(0)->getType()->isIntegerTy();
	else if (dyn_cast<BitCastInst>(I))
		//Only do the instrumentation of the bitcast if the cast is to a lower size datatype (semantically equivalent to a trunc)
		return TruncInstrumentation && I->getType()->isIntegerTy() && I->getOperand(0)->getType()->isIntegerTy() && I->getType()->getPrimitiveSizeInBits() < I->getOperand(0)->getType()->getPrimitiveSizeInBits();
	else
		return false;
}

bool OverflowDetect::runOnModule(Module &M) {

	this->module = &M;
	this->context = &M.getContext();
	this->constZero = NULL;
	SourceFiles.clear();
	
	InterProceduralRA<Cousot> *ra;

	if (UseRaPrunning) {
		ra = &getAnalysis<InterProceduralRA<Cousot> >();

		this->Min = ra->getMin();
		this->Max = ra->getMax();
	}

	NrSignedInsts = 0;
	NrUnsignedInsts = 0;
	NrInsts = 0;
	NrOvfStaticallyDetected = 0;
	NrPossibleOvfStaticallyDetected = 0;
	NrPrunnedInsts = 0;

	//Insert the global declarations (fPrintf, stderr, etc...)
	InsertGlobalDeclarations();

	// Iterate through functions
	for (Module::iterator Fit = M.begin(), Fend = M.end(); Fit != Fend; ++Fit) {
		
		// If the function is empty, do not insert the instrumentation
		// Empty functions include externally linked ones (i.e. abort, printf, scanf, ...)
		if (Fit->begin() == Fit->end())
			continue;
		
		BasicBlock *AbortBB = NULL;
		if (InsertAborts){
			// Create the basic block which the controw flow goes to when the assertion fail
			AbortBB = BasicBlock::Create(*context, "assert fail", dyn_cast<Function>(Fit));
	
			// Call to abort function
			CallInst *abort = CallInst::Create(AbortF, Twine(), AbortBB);
	
			// Add atributes to the abort call instruction: no return and no unwind
			abort->addAttribute(~0, Attribute::NoReturn);
			abort->addAttribute(~0, Attribute::NoUnwind);
	
			// Unreachable instruction
			new UnreachableInst(*context, AbortBB);
		}
		
		// Iterate through basic blocks		
		for (Function::iterator BBit = Fit->begin(), BBend = Fit->end(); BBit != BBend; ++BBit) {

			// Iterate through instructions
			for (BasicBlock::iterator Iit = BBit->begin(); Iit != BBit->end(); ++Iit) {
				
				Instruction* I = dyn_cast<Instruction>(Iit);

				NrInsts++;

				if (isValidInst(I) && !IsNotOriginal(*I)){

					OvfPrediction Pred = ovfStaticAnalysis(I, ra);

					if (Pred != OvWillNotHappen)
						insertInstrumentation(I, AbortBB, Pred);

				}					
			}
		}
	}
    
    // Returns true if the pass make any change to the program
    return (NrSignedInsts + NrUnsignedInsts > 0);
}


void OverflowDetect::insertInstrumentation(Instruction* I, BasicBlock* AbortBB, OvfPrediction Pred){

	// Create comparison instructions, according to the may-overflow instruction.
	// They are inserted just after the instruction I
    Instruction* nextInstruction = GetNextInstruction(*I);
    BasicBlock::iterator nextIt(nextInstruction);


    ICmpInst *positiveOp1;
    ICmpInst *positiveOp2;

    ICmpInst *negativeOp1;
    ICmpInst *negativeOp2;

    ICmpInst *lessThanOp1;
    ICmpInst *lessThanOp2;

    ICmpInst *negativeResult;
    ICmpInst *positiveResult;

    BasicBlock *newBB, *newBB2, *NextBB;
    BranchInst *branch;

	bool isBinaryOperator = (dyn_cast<BinaryOperator>(I) != NULL);
	bool isSigned = isSignedInst(I);

	if (isSigned) {
		NrSignedInsts++;
	} else {
		NrUnsignedInsts++;
	}


	Value* op1;
	Value* op2;

	Value* hasIntegerBug1 = NULL;
	Value* hasIntegerBug2 = NULL;
	Value* hasIntegerBug = NULL;
    Value* canCauseOverflow;
    Value* tmpValue;

    constZero = ConstantInt::get(I->getType(), 0);

	if (isBinaryOperator){
		op1 = I->getOperand(0);
		op2 = I->getOperand(1);
	} else {
		op1 = I->getOperand(0);
	}

	Value* messagePtr = (Pred==OvUnknown ? overflowMessagePtr : overflowMessagePtr2);

	switch(I->getOpcode()){

		case Instruction::Add:

			if (isSigned){
				//How to do an integer bug with a signed Add instruction:
				//		add two big positive numbers and get a negative number as result
				positiveOp1 = new ICmpInst(nextInstruction, CmpInst::ICMP_SGE, op1, constZero);
				positiveOp2 = new ICmpInst(nextInstruction, CmpInst::ICMP_SGE, op2, constZero);
				canCauseOverflow = BinaryOperator::Create(Instruction::And, positiveOp1, positiveOp2, "", nextInstruction);
				negativeResult = new ICmpInst(nextInstruction, CmpInst::ICMP_SLT, I, constZero);
				hasIntegerBug1 = BinaryOperator::Create(Instruction::And, canCauseOverflow, negativeResult, "", nextInstruction);

				//	or	add two big negative numbers and get a positive number as result
				negativeOp1 = new ICmpInst(nextInstruction, CmpInst::ICMP_SLT, op1, constZero);
				negativeOp2 = new ICmpInst(nextInstruction, CmpInst::ICMP_SLT, op2, constZero);
				canCauseOverflow = BinaryOperator::Create(Instruction::And, negativeOp1, negativeOp2, "", nextInstruction);
				positiveResult = new ICmpInst(nextInstruction, CmpInst::ICMP_SGE, I, constZero);
				hasIntegerBug2 = BinaryOperator::Create(Instruction::And, canCauseOverflow, positiveResult, "", nextInstruction);

				// AND the results
				hasIntegerBug = BinaryOperator::Create(Instruction::Or, hasIntegerBug1, hasIntegerBug2, "", nextInstruction);
			} else {
				//How to do an integer bug with a unsigned Add instruction:
				//		add two numbers and get a result smaller than one of the operands
				lessThanOp1 = new ICmpInst(nextInstruction, CmpInst::ICMP_ULT, dyn_cast<Value>(I), op1);
				lessThanOp2 = new ICmpInst(nextInstruction, CmpInst::ICMP_ULT, dyn_cast<Value>(I), op2);
				hasIntegerBug = BinaryOperator::Create(Instruction::Or, lessThanOp1, lessThanOp2, "", nextInstruction);
			}
			break;

		case Instruction::Sub:

			if (isSigned) {
				//How to do an integer bug with a signed Sub instruction:
				//		subtract a positive number from a negative number and get a positive number as result
				negativeOp1 = new ICmpInst(nextInstruction, CmpInst::ICMP_SLT, op1, constZero);
				positiveOp2 = new ICmpInst(nextInstruction, CmpInst::ICMP_SGE, op2, constZero);
				canCauseOverflow = BinaryOperator::Create(Instruction::And, negativeOp1, positiveOp2, "", nextInstruction);
				positiveResult = new ICmpInst(nextInstruction, CmpInst::ICMP_SGE, I, constZero);
				hasIntegerBug1 = BinaryOperator::Create(Instruction::And, canCauseOverflow, positiveResult, "", nextInstruction);

				//	or	subtract a negative number from a positive number and get a negative number as result
				positiveOp1 = new ICmpInst(nextInstruction, CmpInst::ICMP_SGE, op1, constZero);
				negativeOp2 = new ICmpInst(nextInstruction, CmpInst::ICMP_SLT, op2, constZero);
				canCauseOverflow = BinaryOperator::Create(Instruction::And, positiveOp1, negativeOp2, "", nextInstruction);
				negativeResult = new ICmpInst(nextInstruction, CmpInst::ICMP_SLT, I, constZero);
				hasIntegerBug2 = BinaryOperator::Create(Instruction::And, canCauseOverflow, negativeResult, "", nextInstruction);

				// AND the results
				hasIntegerBug = BinaryOperator::Create(Instruction::Or, hasIntegerBug1, hasIntegerBug2, "", nextInstruction);

			} else {

				//How to do an integer bug with an unsigned Sub instruction:
				//		subtract a number from a smaller number. It should result a negative number, but Unsigned numbers
				//		don't store negative numbers. There is our overflow.
				hasIntegerBug = new ICmpInst(nextInstruction, CmpInst::ICMP_ULT, op1, op2);

			}
			break;

		case Instruction::Mul:

			//How to verify if an integer bug has just happened :
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
				hasIntegerBug = new ICmpInst(branch, CmpInst::ICMP_NE, tmpValue, op2);

			} else {

				tmpValue = BinaryOperator::Create(Instruction::UDiv, I, op1, "", branch);
				hasIntegerBug = new ICmpInst(branch, CmpInst::ICMP_NE, tmpValue, op2);

			}

			// Remove the unconditional branch created by splitBasicBlock, and insert a conditional
			// branch that correctly connects to newBB and assertfail
			branch = cast<BranchInst>(newBB2->getTerminator());
			branch->eraseFromParent();

			NextBB = InsertAborts ? AbortBB : newBB;

			BranchInst::Create(NewOverflowOccurrenceBlock(I, NextBB, messagePtr), newBB, hasIntegerBug, newBB2);

			break;

		case Instruction::Shl:

			//The result of a Shift Left must be positive if the first operand is positive
			//    or the second operand must be zero if the first operand is negative.
			if (isSigned){


				positiveOp1 = new ICmpInst(nextInstruction, CmpInst::ICMP_SGE, op1, constZero);
				negativeResult = new ICmpInst(nextInstruction, CmpInst::ICMP_SLT, I, constZero);
				hasIntegerBug1 = BinaryOperator::Create(Instruction::And, positiveOp1, negativeResult, "", nextInstruction);

				negativeOp1 = new ICmpInst(nextInstruction, CmpInst::ICMP_SLT, op1, constZero);
				canCauseOverflow = new ICmpInst(nextInstruction, CmpInst::ICMP_NE, op2, constZero);
				hasIntegerBug1 = BinaryOperator::Create(Instruction::And, canCauseOverflow, negativeOp1, "", nextInstruction);

				hasIntegerBug = BinaryOperator::Create(Instruction::Or, hasIntegerBug1, hasIntegerBug2, "", nextInstruction);


			} else {

				//if the result is greater than the first operand, something has gone wrong in the SHL
				hasIntegerBug = new ICmpInst(nextInstruction, CmpInst::ICMP_ULT, I, op1);

			}
			break;

		case Instruction::BitCast:
		case Instruction::Trunc:
			/*
			 * How to check an integer bug in a trunc instruction:
			 * 		Cast the truncated value back to its original type and check if the value remains equal
			 */
			messagePtr = (Pred==OvUnknown ? truncErrorMessagePtr : truncErrorMessagePtr2);
			if (isSigned){

				tmpValue = new SExtInst(I, op1->getType(), "", nextInstruction);
				hasIntegerBug = new ICmpInst(nextInstruction, CmpInst::ICMP_NE, tmpValue, op1);

			} else {

				tmpValue = CastInst::CreateZExtOrBitCast(I, op1->getType(), "", nextInstruction);
				MarkAsNotOriginal(*(dyn_cast<Instruction>(tmpValue)));
				hasIntegerBug = new ICmpInst(nextInstruction, CmpInst::ICMP_NE, tmpValue, op1);

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

		NextBB = InsertAborts ? AbortBB : newBB;

		branch = BranchInst::Create(NewOverflowOccurrenceBlock(I, NextBB, messagePtr), newBB, hasIntegerBug, I->getParent());

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
