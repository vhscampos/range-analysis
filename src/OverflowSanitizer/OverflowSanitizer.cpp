/*
 * OverflowSanitizer.cpp
 *
 *  Created on: Apr 09, 2013
 *      Author: raphael
 */
#define DEBUG_TYPE "OverflowSanitizer"

#include "OverflowSanitizer.h"

using namespace std;

#define InsertAborts false
#define TruncInstrumentation true
#define InsertFprintfs false


static cl::opt<bool, false>
OnlySLE("only-SLE", cl::desc("Only consider SLE, ULE, SGE and UGE as loop conditions."), cl::NotHidden);

//Table 1
STATISTIC(NrLoops					, "Number of Loops");
STATISTIC(InpDepLoops				, "Number of Input-dependent Loops");
STATISTIC(VulnerableLoops			, "Number of Vulnerable Loops");
STATISTIC(AverageDependencyDistance	, "Average Dependency Distance");

//Table 2
STATISTIC(NumInstructionsBefore					, "Number of Instructions Before Instrumentation");
STATISTIC(NumOvfInstructions					, "Number of may-overflow Instructions");
STATISTIC(NumInstrumentedInsts					, "Number of instrumented Instructions");
STATISTIC(NumInstructionsAfter					, "Number of Instructions After Instrumentation");


/*
 * 	Instructions that may cause an overflow
 */
bool OverflowSanitizer::isValidInst(Instruction *I)
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

std::set<llvm::Value*> visitedValues;


void llvm::OverflowSanitizer::lookForValuesToSafe(Instruction* I, BasicBlock* loopHeader) {


	if (visitedValues.count(I)) return;
	visitedValues.insert(I);

	//Check if the instruction can overflow and it is not in the list
	if (isValidInst(I) && !valuesToSafe.count(I)) {
		valuesToSafe.insert(I);

		if (!vulnerableLoops.count(loopHeader)){

			vulnerableLoops.insert(loopHeader);
		}
	}

	//Look for more instructions in the operands
	for (unsigned int i = 0; i < I->getNumOperands(); i++){

		if (Instruction* Inst = dyn_cast<Instruction>(I->getOperand(i))) {

			BasicBlock* parentBB = Inst->getParent();

			if ( loopBlocks.count(parentBB) ) {

				if ( isReachable(parentBB, loopHeader)  ) {

					lookForValuesToSafe(Inst, loopHeader);

				}

			}

		}

	}

}

bool basicBlockSearch(BasicBlock* src, BasicBlock* dst, std::list<BasicBlock*> &blocksToVisit, std::set<BasicBlock*> &visitedBlocks){

	if (src == dst) return true;

	visitedBlocks.insert(src);

	//Insert the new blocks to visit in an order to allow us making a DFS
	for (succ_iterator PI = succ_begin(src), E = succ_end(src); PI != E; ++PI) {

		BasicBlock *succBB = *PI;
		blocksToVisit.push_front(succBB);

	}

	if (blocksToVisit.size() == 0) return false;

	BasicBlock* src2 = NULL;

	//Remove items from the queue not allowing to visit the same block twice
	while (src2 == NULL && blocksToVisit.size() > 0 ) {

		if (!visitedBlocks.count(blocksToVisit.front())) {
			src2 = blocksToVisit.front();
		}

		blocksToVisit.pop_front();

	}

	return (src2 == NULL ? false : basicBlockSearch(src2, dst, blocksToVisit, visitedBlocks));


}


bool llvm::OverflowSanitizer::isReachable(BasicBlock* src, BasicBlock* dst) {

	if (loopBlocks.count(src) && loopBlocks.count(dst))
		if (loopBlocks[src] == loopBlocks[dst]) // if both blocks belong to the same loop, one can rech the otherand we don't need to do the DFS
			return true;

	std::list<BasicBlock*> blocksToVisit;
	std::set<BasicBlock*> visitedBlocks;
	return basicBlockSearch(src, dst, blocksToVisit, visitedBlocks);

}


BasicBlock* OverflowSanitizer::NewOverflowOccurrenceBlock(Instruction* I, BasicBlock* NextBlock, Value *messagePtr){

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
		ArrayRef<Value*> ARargs(args);
		CallInst::Create(FPrintF, ARargs, "", branch);
	}

	return result;
}

int llvm::OverflowSanitizer::countInstructions() {

	int result = 0;

	for (Module::iterator Fit = module->begin(), Fend = module->end(); Fit != Fend; Fit++){
		for (Function::iterator BBit = Fit->begin(), BBend = Fit->end(); BBit != BBend; BBit++){
			result += BBit->size();
		}
	}

	return result;

}

int llvm::OverflowSanitizer::countOverflowableInsts() {

	int result = 0;

	for (Module::iterator Fit = module->begin(), Fend = module->end(); Fit != Fend; Fit++){
		for (Function::iterator BBit = Fit->begin(), BBend = Fit->end(); BBit != BBend; BBit++){
			for(BasicBlock::iterator I = BBit->begin(), E = BBit->end(); I != E; I++){
				if (isValidInst(I)) result++;
			}
		}
	}

	return result;

}

bool ::OverflowSanitizer::runOnModule(Module& M) {

	module = &M;
	context = &M.getContext();
	int totalDistance = 0;


	NumInstructionsBefore = countInstructions();
	NumOvfInstructions = countOverflowableInsts();

	analyzeLoops();
	NrLoops = loopHeaders.size();

	InsertGlobalDeclarations();

	InputValues& iVAnalysis = getAnalysis<InputValues>();
	inputValues = iVAnalysis.getInputDepValues();

	moduleDepGraph& depGraph = getAnalysis<moduleDepGraph>();

	std::set<Loop*> inputDependentLoops;

	//Check the branch condition of each loop header. Mark the loop headers that are reachable by Inputs.
	for (std::map<BasicBlock*, LoopInfo*>::iterator Bit = loopExitBlocks.begin(), Bend = loopExitBlocks.end(); Bit != Bend; Bit++){

		if (BranchInst* BI = dyn_cast<BranchInst>((Bit->first)->getTerminator())) {

			if (BI->isConditional()){

				Value* loopCondition = BI->getCondition();

				std::pair<GraphNode*,int> nearestDependency = depGraph.depGraph->getNearestDependency(loopCondition, inputValues, false);

				if(nearestDependency.first){

					if (!inputDependentLoops.count(loopBlocks[Bit->first])) {
						InpDepLoops++;
						inputDependentLoops.insert(loopBlocks[Bit->first]);
					}

					totalDistance += nearestDependency.second;
					inpDepLoopConditions.push_back(loopCondition);

				}

			}

		}

	}

	if (InpDepLoops) AverageDependencyDistance = totalDistance/inpDepLoopConditions.size();

	//Verify the input-dependent loop headers that are controlled by SLE, SGE, ULE, UGE ICmpInsts. Mark them as vulnerable
	for (std::list<Value*>::iterator Cit = inpDepLoopConditions.begin(), Cend = inpDepLoopConditions.end(); Cit != Cend; Cit++){


		if (OnlySLE) {

			//The ICmpInst has 2 operands: X comes from input. Y is updated inside the loop. We must make sure that Y will never overflow.
			if (ICmpInst* CI = dyn_cast<ICmpInst>(*Cit)) {

				BasicBlock* parentBB = CI->getParent();
				BasicBlock* loopHeader = loopHeaders[loopBlocks[parentBB]];

				switch (CI->getPredicate()){

					case CmpInst::ICMP_SLE:
					case CmpInst::ICMP_ULE:
					case CmpInst::ICMP_SGE:
					case CmpInst::ICMP_UGE:

						lookForValuesToSafe(CI, loopHeader);

						break;
					default:
						break;
				}

			}

		} else {

			if (Instruction* Inst = dyn_cast<Instruction>(*Cit)) {

				BasicBlock* parentBB = Inst->getParent();
				BasicBlock* loopHeader = loopHeaders[loopBlocks[parentBB]];

				lookForValuesToSafe(Inst, loopHeader);

			}

		}

	}

	VulnerableLoops = vulnerableLoops.size();

	NumInstrumentedInsts = valuesToSafe.size();

	//insert instrumentation

	for( std::set<Instruction*>::iterator i = valuesToSafe.begin(), e = valuesToSafe.end(); i != e; i++){

		Function* F = (*i)->getParent()->getParent();

		BasicBlock *AbortBB = NULL;

		if (abortBlocks.count(F)) {
			AbortBB = abortBlocks[F];
		} else {

			// Create the basic block which the control flow goes to when the assertion fail
			AbortBB = BasicBlock::Create(*context, "assert fail", F);

			// Call to abort function
			CallInst *abort = CallInst::Create(AbortF, Twine(), AbortBB);

			// Add attributes to the abort call instruction: no return and no unwind
			abort->addAttribute(~0, Attribute::NoReturn);
			abort->addAttribute(~0, Attribute::NoUnwind);

			// Unreachable instruction
			new UnreachableInst(*context, AbortBB);

			abortBlocks[F] = AbortBB;

		}

		insertInstrumentation(*i, AbortBB, OvUnknown);
	}

	NumInstructionsAfter = countInstructions();

	return true;
}




void OverflowSanitizer::MarkAsNotOriginal(Instruction& inst)
{
	inst.setMetadata("new-inst", MDNode::get(*context, llvm::ArrayRef<Value*>()));
}
bool OverflowSanitizer::IsNotOriginal(Instruction& inst)
{
	return inst.getMetadata("new-inst") != 0;
}


/*
 * Method that collects the loop headers of the program
 */
void ::OverflowSanitizer::analyzeLoops() {

	std::set<BasicBlock*> visitedBlocks;
	std::set<BasicBlock*> blocksToVisit;

	for (Module::iterator Fit = module->begin(), Fend = module->end(); Fit != Fend; Fit++){

		if (Fit->begin() == Fit->end()) continue;

		LoopInfo& li = getAnalysis<LoopInfo>(*dyn_cast<Function>(Fit));

		for (Function::iterator BBit = Fit->begin(), BBend = Fit->end(); BBit != BBend; BBit++){

			Loop* CurrentLoop = li.getLoopFor(BBit);
			if (li.isLoopHeader(BBit)) loopHeaders[CurrentLoop] = BBit;

			//FIXME this is not the correct way to get the exit blocks. We are getting all the blocks that
			//		have a successor outside the loop
			if (CurrentLoop){
				loopBlocks[BBit] = CurrentLoop;

				unsigned int CurrentDepth = li.getLoopDepth(BBit);

				for (succ_iterator PI = succ_begin(BBit), E = succ_end(BBit); PI != E; ++PI) {

					BasicBlock *succBB = *PI;

					if (li.getLoopDepth(succBB) < CurrentDepth) {
						loopExitBlocks[BBit] = &li;
						break;
					}

				}

			}

		}

	}

}

Constant* OverflowSanitizer::strToLLVMConstant(std::string s){

	std::vector<unsigned char> vec( s.begin(), s.end() );

	std::vector<Constant*>	cVec;

	for(unsigned int i = 0; i < vec.size(); i++){
		cVec.push_back( llvm::ConstantInt::get(Type::getInt8Ty(*context), APInt(8, vec[i], false) ) );
	}

	llvm::ArrayRef<Constant*> aRef(cVec);

	int size = vec.size();
	ArrayType* arrayType = ArrayType::get(Type::getInt8Ty(*context), size);

	return llvm::ConstantArray::get( arrayType, aRef);

}


/*
 * Inserts global variable and function declarations, needed during the instrumentation.
 */
void OverflowSanitizer::InsertGlobalDeclarations(){

	//Create a global variable with the fprintf Message
	std::string message1("Overflow occurred in %s, line %d. [%d]\n");
	std::string message2("Truncation with data loss occurred in %s, line %d. [%d]\n");
	std::string message3("(Suspected) Overflow occurred in %s, line %d. [%d]\n");
	std::string message4("(Suspected) Truncation with data loss occurred in %s, line %d. [%d]\n");



	Constant* stringConstant = strToLLVMConstant(message1);
	GlobalVariable* messageStr = new GlobalVariable(*module, stringConstant->getType(), true,
	                                                llvm::GlobalValue::InternalLinkage,
	                                                stringConstant, "OverflowMessage");

	//Get the int8ptr to our message
    constZero = ConstantInt::get(Type::getInt32Ty(*context), 0);
	Constant* constArray = ConstantExpr::getInBoundsGetElementPtr(messageStr, constZero);
	overflowMessagePtr = ConstantExpr::getBitCast(constArray, PointerType::getUnqual(Type::getInt8Ty(*context)));


	stringConstant = strToLLVMConstant(message2);
	messageStr = new GlobalVariable(*module, stringConstant->getType(), true,
	                                                llvm::GlobalValue::InternalLinkage,
	                                                stringConstant, "TruncErrorMessage");

	//Get the int8ptr to our message
	constArray = ConstantExpr::getInBoundsGetElementPtr(messageStr, constZero);
	truncErrorMessagePtr = ConstantExpr::getBitCast(constArray, PointerType::getUnqual(Type::getInt8Ty(*context)));


	//Messages for the overflows statically detected (suspect instructions)
	stringConstant = strToLLVMConstant(message3);
	messageStr = new GlobalVariable(*module, stringConstant->getType(), true,
	                                                llvm::GlobalValue::InternalLinkage,
	                                                stringConstant, "OverflowMessage");

	//Get the int8ptr to our message
	constArray = ConstantExpr::getInBoundsGetElementPtr(messageStr, constZero);
	overflowMessagePtr2 = ConstantExpr::getBitCast(constArray, PointerType::getUnqual(Type::getInt8Ty(*context)));

	stringConstant = strToLLVMConstant(message4);
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
		ArrayRef<Type*> aRElements(Elements);
		IO_FILE_ty->setBody(aRElements, false);

		std::vector<Type*> Elements2;
		Elements2.push_back(IO_marker_ptr_ty);
		Elements2.push_back(IO_FILE_PTR_ty);
		Elements2.push_back(Type::getInt32Ty(*context));;
		ArrayRef<Type*> aRElements2(Elements2);
		IO_marker_ty->setBody(aRElements2, false);

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
    ArrayRef<Type*> aRParams(Params);

    // Get the fprintf() function (takes an IO_FILE* and an i8* followed by variadic parameters)
    FPrintF = module->getOrInsertFunction("fprintf",FunctionType::get(Type::getVoidTy(*context), aRParams, true));


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


static bool isSignedInst(llvm::Value* V){
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

void OverflowSanitizer::insertInstrumentation(Instruction* I, BasicBlock* AbortBB, OvfPrediction Pred){

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

//	if (isSigned) {
//		NrSignedInsts++;
//	} else {
//		NrUnsignedInsts++;
//	}


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
		// This new BB is where control flow goes to when the assertion is correct
		newBB = I->getParent()->splitBasicBlock(nextIt);

		// Remove the unconditional branch created by splitBasicBlock, and insert a conditional
		// branch that correctly connects to newBB and assertfail
		branch = cast<BranchInst>(I->getParent()->getTerminator());
		branch->eraseFromParent();

		NextBB = InsertAborts ? AbortBB : newBB;

		branch = BranchInst::Create(NewOverflowOccurrenceBlock(I, NextBB, messagePtr), newBB, hasIntegerBug, I->getParent());

	}
}

Instruction* OverflowSanitizer::GetNextInstruction(Instruction& i)
{
	BasicBlock::iterator it(&i);
	it++;
	return it;
}


Constant* OverflowSanitizer::getSourceFile(Instruction* I){

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
		Constant* stringConstant = strToLLVMConstant(File);
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

Constant* OverflowSanitizer::getLineNumber(Instruction* I){

	if (MDNode *N = I->getMetadata("dbg")) {
	    DILocation Loc(N);
	    unsigned Line = Loc.getLineNumber();
	    return ConstantInt::get(Type::getInt32Ty(*context), Line);
	}	else {
		return ConstantInt::get(Type::getInt32Ty(*context), 0);
	}

}

char OverflowSanitizer::ID = 0;
static RegisterPass<OverflowSanitizer> Y("overflow-sanitizer", "Overflow Sanitizer Pass");
