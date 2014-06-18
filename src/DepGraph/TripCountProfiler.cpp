/*
 * TripCountProfiler.cpp
 *
 *  Created on: Dec 10, 2013
 *      Author: raphael
 */

#ifndef DEBUG_TYPE
#define DEBUG_TYPE "TripCountProfiler"
#endif

#include "TripCountProfiler.h"

STATISTIC(NumInstrumentedLoops, "Number of Instrumented Loops");
STATISTIC(NumUnknownTripCount,	"Number of Unknown Estimated Trip Count");
STATISTIC(NumIgnoredLoops,		"Number of Ignored Loops");


using namespace llvm;


AllocaInst* insertAlloca(BasicBlock *BB, Constant* initialValue) {
  Type* Ty = Type::getInt64Ty(BB->getParent()->getContext());
  AllocaInst* A = new AllocaInst(Ty, "", BB->getFirstNonPHI());
  new StoreInst(initialValue, A, false, 4, BB->getTerminator());
  return A;
}

Value* insertAdd(BasicBlock *BB, AllocaInst *A) {
  IRBuilder<> Builder(BB->getFirstNonPHI());
  Value* L = Builder.CreateAlignedLoad(A, 4);
  ConstantInt* One = ConstantInt::get(Type::getInt64Ty(BB->getParent()->getContext()), 1);
  Value* I = Builder.CreateAdd(L, One);
  Builder.CreateAlignedStore(I, A, 4);
  return I;
}

void llvm::TripCountProfiler::saveTripCount(std::set<BasicBlock*> BBs, AllocaInst* tripCountPtr, Value* estimatedTripCount,  BasicBlock* loopHeader, int LoopClass){


	for(std::set<BasicBlock*>::iterator it = BBs.begin(), end = BBs.end(); it != end; it++){

		BasicBlock* BB = *it;

		IRBuilder<> Builder(BB->getFirstInsertionPt());

		ConstantInt* loopIdentifier = ConstantInt::get(Type::getInt64Ty(*context), (int64_t)loopHeader);
		ConstantInt* loopClass = ConstantInt::get(Type::getInt32Ty(*context), (int64_t)LoopClass);


		//Value* stderr = Builder.CreateAlignedLoad(GVstderr, 4);
		Value* tripCount = Builder.CreateAlignedLoad(tripCountPtr, 4);

		std::vector<Value*> args;
		args.push_back(loopIdentifier);
		args.push_back(tripCount);
		args.push_back(estimatedTripCount);
		args.push_back(loopClass);
		llvm::ArrayRef<llvm::Value *> arrayArgs(args);
		Builder.CreateCall(collectLoopData, arrayArgs, "");


		int count = 0;
		for(pred_iterator pred = pred_begin(BB); pred != pred_end(BB); pred++) {
			count ++;
		}

	}

}




bool llvm::TripCountProfiler::doInitialization(Module& M) {

	context = &M.getContext();

	NumInstrumentedLoops = 0;
	NumUnknownTripCount = 0;
	NumIgnoredLoops = 0;



	/*
	 * We will insert calls to functions in specific
	 * points of the program.
	 *
	 * Before doing that, we must declare the functions.
	 * Here we have our declarations.
	 */

	Type* Ty = Type::getVoidTy(*context);
	FunctionType *T = FunctionType::get(Ty, true);
	initLoopList = M.getOrInsertFunction("initLoopList", T);

	std::vector<Type*> args;
	args.push_back(Type::getInt64Ty(*context));   // Loop Identifier
	args.push_back(Type::getInt64Ty(*context));   // Actual TripCount
	args.push_back(Type::getInt64Ty(*context));   // Estimated TripCount
	args.push_back(Type::getInt32Ty(*context));   // LoopClass (0=Interval; 1=Equality; 2=Other)
	llvm::ArrayRef<Type*> arrayArgs(args);
	FunctionType *T1 = FunctionType::get(Ty, arrayArgs, true);
	collectLoopData = M.getOrInsertFunction("collectLoopData", T1);

	std::vector<Type*> args2;
	args2.push_back(Type::getInt8PtrTy(*context));
	llvm::ArrayRef<Type*> arrayArgs2(args2);
	FunctionType *T2 = FunctionType::get(Ty, arrayArgs2, true);
	flushLoopStats = M.getOrInsertFunction("flushLoopStats", T2);

	return true;
}


Instruction* getNextInstruction(Instruction* I){

	Instruction* result = NULL;

	if(!isa<TerminatorInst>(I)){
		BasicBlock::iterator it(I);
		it++;
		result = it;
	}

	return result;

}

Value* TripCountProfiler::getValueAtEntryPoint(Value* source, BasicBlock* loopHeader){

	LoopInfoEx& li = getAnalysis<LoopInfoEx>();
	LoopNormalizerAnalysis& ln = getAnalysis<LoopNormalizerAnalysis>();

	Loop* loop = li.getLoopFor(loopHeader);

	//Option 1: Loop invariant. Return the value itself
	if (loop->isLoopInvariant(source)) return source;

	//Option 2: Sequence of redefinitions with PHI node in the loop header. Return the incoming value from the entry block
	LoopControllersDepGraph& lcd = getAnalysis<LoopControllersDepGraph>();
	GraphNode* node = lcd.depGraph->findNode(source);
	if (!node) {
		return NULL;
	}

	int SCCID = lcd.depGraph->getSCCID(node);
	DepGraph sccGraph = lcd.depGraph->generateSubGraph(SCCID);

	for(DepGraph::iterator it =  sccGraph.begin(); it != sccGraph.end(); it++){

		Value* V = NULL;

		if (VarNode* VN = dyn_cast<VarNode>(*it)) {
			V = VN->getValue();
		} else	if (OpNode* ON = dyn_cast<OpNode>(*it)) {
			V = ON->getOperation();
		}

		if (V) {
			if (PHINode* PHI = dyn_cast<PHINode>(V)) {
				if(PHI->getParent() == loopHeader ) {

					Value* IncomingFromEntry = PHI->getIncomingValueForBlock(ln.entryBlocks[loopHeader]);
					return IncomingFromEntry;

				}
			}
		}
	}

	Instruction *InstToCopy = NULL;

	//Option 3: Sequence of loads/stores in the same memory location. Create load in the entry block and return the loaded value
	if (LoadInst* LI = dyn_cast<LoadInst>(source)){
		InstToCopy = LI;
	}


	//Option 4: Cast Instruction (Bitcast, Zext, SExt, Trunc etc...): Propagate search (The value theoretically is the same)
	if (CastInst* CI = dyn_cast<CastInst>(source)){
		return getValueAtEntryPoint(CI->getOperand(0), loopHeader);
	}

	//Option 5: GetElementPTR - Create a similar getElementPtr in the entry block
	if (GetElementPtrInst* GEPI = dyn_cast<GetElementPtrInst>(source)){

		// Do the copy only if all the operands are loop-invariant
		bool isInvariant = true;

		for(unsigned int i = 0; i < GEPI->getNumOperands(); i++){
			if (!loop->isLoopInvariant(GEPI->getOperand(i))) {
				isInvariant = false;
				break;
			}
		}

		if (isInvariant) InstToCopy = GEPI;
	}



	//Here we try to copy the instruction in the entry block
	//we adjust the operands  to the value dominate all of its uses.
	if (InstToCopy) {

		unsigned int prev_size = ln.entryBlocks[loopHeader]->getInstList().size();

		Instruction* NEW_INST = InstToCopy->clone();
				ln.entryBlocks[loopHeader]->getInstList().insert(ln.entryBlocks[loopHeader]->getFirstInsertionPt(), NEW_INST);


		for(unsigned int i = 0; i < InstToCopy->getNumOperands(); i++){

			Value* op = getValueAtEntryPoint(InstToCopy->getOperand(i), loopHeader);

			if (op){
				if (op->getType() != InstToCopy->getOperand(i)->getType()) op = NULL;
			}

			if (!op) {

				//Undo changes in the entry block
				while (ln.entryBlocks[loopHeader]->getInstList().size() > prev_size) {
					ln.entryBlocks[loopHeader]->getFirstInsertionPt()->eraseFromParent();
				}

				return NULL;
			}

			NEW_INST->setOperand(i, op);
		}

		return NEW_INST;
	}

	//Option 9999: unknown. Return NULL
	return NULL;
}


/*
 * Given a natural loop, find the basic block that is more likely block that
 * controls the number of iteration of a loop.
 *
 * In for-loops and while-loops, the loop header is the controller, for instance
 * In repeat-until-loops, the loop controller is a basic block that has a successor
 * 		outside the loop and the loop header as a successor.
 *
 *  Otherwise, return the first exit block
 */
BasicBlock* TripCountProfiler::findLoopControllerBlock(Loop* l){

	BasicBlock* header = l->getHeader();

	SmallVector<BasicBlock*, 2>  exitBlocks;
	l->getExitingBlocks(exitBlocks);

	//Case 1: for/while (the header is an exiting block)
	for (SmallVectorImpl<BasicBlock*>::iterator It = exitBlocks.begin(), Iend = exitBlocks.end(); It != Iend; It++){
		BasicBlock* BB = *It;
		if (BB == header) {
			return BB;
		}
	}

	//Case 2: repeat-until/do-while (the exiting block can branch directly to the header)
	for (SmallVectorImpl<BasicBlock*>::iterator It = exitBlocks.begin(), Iend = exitBlocks.end(); It != Iend; It++){

		BasicBlock* BB = *It;

		//Here we iterate over the successors of BB to check if it is a block that leads the control
		//back to the header.
		for(succ_iterator s = succ_begin(BB); s != succ_end(BB); s++){

			if (*s == header) {
				return BB;
			}
		}

	}

	//Otherwise, return the first exit block
	return *(exitBlocks.begin());
}

bool isIntervalComparison(ICmpInst* CI){

	switch(CI->getPredicate()){
	case ICmpInst::ICMP_SGE:
	case ICmpInst::ICMP_SGT:
	case ICmpInst::ICMP_UGE:
	case ICmpInst::ICMP_UGT:
	case ICmpInst::ICMP_SLE:
	case ICmpInst::ICMP_SLT:
	case ICmpInst::ICMP_ULE:
	case ICmpInst::ICMP_ULT:
		return true;
		break;
	default:
		return false;
	}

}



bool TripCountProfiler::runOnFunction(Function &F){

	IRBuilder<> Builder(F.getEntryBlock().getTerminator());

	if (!moduleIdentifierStr){
		moduleIdentifierStr = Builder.CreateGlobalStringPtr(F.getParent()->getModuleIdentifier(), "moduleIdentifierStr");
	}

	Value* main = F.getParent()->getFunction("main");
	if(!main) main = F.getParent()->getFunction("MAIN__"); //Fortan hack

	bool isMain = (&F == main);

	if (isMain){
		Builder.CreateCall(initLoopList, "");
	}


	if (&F ==  F.getParent()->getFunction("P7Traces2Alignment")){

		errs() << F << "\n";
		return false;

	}



	LoopInfoEx& li = getAnalysis<LoopInfoEx>();
	TripCountAnalysis& tca = getAnalysis<TripCountAnalysis>();


	/*
	 * Here we have all the instructions that will stop the program
	 *
	 * E.g.: abort, exit, return of function main
	 *
	 * Before those instructions, we will print all the data we have collected.
	 */
	ExitInfo& eI = getAnalysis<ExitInfo>();
	for(std::set<Instruction*>::iterator Iit = eI.exitPoints.begin(), Iend = eI.exitPoints.end(); Iit != Iend; Iit++){

		Instruction* I = *Iit;

		if(I->getParent()->getParent() == &F){
			Builder.SetInsertPoint(I);

			std::vector<Value*> args;
			args.push_back(moduleIdentifierStr);
			llvm::ArrayRef<llvm::Value *> arrayArgs(args);
			Builder.CreateCall(flushLoopStats, arrayArgs, "");
		}

	}



	LoopNormalizerAnalysis& ln = getAnalysis<LoopNormalizerAnalysis>();


	Constant* constZero = ConstantInt::get(Type::getInt64Ty(F.getContext()), 0);

	Constant* unknownTripCount = ConstantInt::get(Type::getInt64Ty(F.getContext()), -2);


	for(LoopInfoEx::iterator lit = li.begin(); lit != li.end(); lit++){


		bool mustInstrument = true;

		Loop* loop = *lit;

		BasicBlock* header = loop->getHeader();
		BasicBlock* entryBlock = ln.entryBlocks[header];

		/*
		 * Here we are looking for the predicate that stops the loop.
		 *
		 * At this moment, we are only considering loops that are controlled by
		 * integer comparisons.
		 */
		BasicBlock* exitBlock = findLoopControllerBlock(loop);
		assert (exitBlock && "Exit block not found!");


		TerminatorInst* T = exitBlock->getTerminator();
		BranchInst* BI = dyn_cast<BranchInst>(T);
		ICmpInst* CI = BI ? dyn_cast<ICmpInst>(BI->getCondition()) : NULL;

		Value* Op1 = NULL;
		Value* Op2 = NULL;

		int LoopClass;

		if (!CI) {
			LoopClass = 2;
			mustInstrument = false;
		}
		else {

			if (isIntervalComparison(CI)) {
				LoopClass = 0;
			} else {
				LoopClass = 1;
			}


			Op1 = getValueAtEntryPoint(CI->getOperand(0), header);
			Op2 = getValueAtEntryPoint(CI->getOperand(1), header);


			if((!Op1) || (!Op2) ) {

			} else if((!Op1->getType()->isIntegerTy()) || (!Op2->getType()->isIntegerTy())) {
				mustInstrument = false;
			}
		}

		Value* estimatedTripCount = tca.getTripCount(header);

		if((!estimatedTripCount) && mustInstrument){
			estimatedTripCount = unknownTripCount;
			LoopClass += 3; // 3 = UnknownIntervalLoop; 4 = UnknownEqualityLoop
			NumUnknownTripCount++;
		}


		if (estimatedTripCount) {

			//Before the loop starts, the trip count is zero
			AllocaInst* tripCount = insertAlloca(entryBlock, constZero);

			//Every time the loop header is executed, we increment the trip count
			insertAdd(header, tripCount);


			/*
			 * We will collect the actual trip count and the estimate trip count in every
			 * basic block that is outside the loop
			 */
			std::set<BasicBlock*> blocksToInstrument;
			SmallVector<BasicBlock*, 2> exitBlocks;
			loop->getExitBlocks(exitBlocks);
			for (SmallVectorImpl<BasicBlock*>::iterator eb = exitBlocks.begin(); eb !=  exitBlocks.end(); eb++){

				BasicBlock* CurrentEB = *eb;

				/*
				 * Does not instrument landingPad (exception handling) blocks
				 * TODO: Handle LandingPad blocks (if possible)
				 */
				if(!CurrentEB->isLandingPad())
					blocksToInstrument.insert(CurrentEB);

			}

			saveTripCount(blocksToInstrument, tripCount, estimatedTripCount, header, LoopClass);

			NumInstrumentedLoops++;

		} else {
			NumIgnoredLoops++;
		}
	}

	return true;

}


char llvm::TripCountProfiler::ID = 0;
static RegisterPass<TripCountProfiler> X("tc-profiler","Trip Count Profiler");
