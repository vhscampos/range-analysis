/*
 * TripCountGenerator.cpp
 *
 *  Created on: Dec 10, 2013
 *      Author: raphael
 */

#ifndef DEBUG_TYPE
#define DEBUG_TYPE "TripCountGenerator"
#endif

#include "TripCountGenerator.h"

static cl::opt<bool, false>
usePericlesTripCount("usePericlesTripCount", cl::desc("Use Pericles's heuristic to estimate trip count."), cl::NotHidden);

static cl::opt<bool, false>
useHybridTripCount("useHybridTripCount", cl::desc("Use Hybrid heuristic (Vector + Pericles) to estimate trip count."), cl::NotHidden);


static cl::opt<bool, false>
estimateMinimumTripCount("estimateMinimumTripCount", cl::desc("Estimate the minimum trip count of the loops."), cl::NotHidden);


STATISTIC(NumVectorEstimatedTCs, 	"Number of Estimated Trip Counts by Vector");
STATISTIC(NumPericlesEstimatedTCs, 	"Number of Estimated Trip Counts by Pericles");

STATISTIC(NumIntervalLoops, "Number of Interval Loops");
STATISTIC(NumEqualityLoops, "Number of Equality Loops");
STATISTIC(NumOtherLoops,    "Number of Other Loops");

STATISTIC(NumUnknownConditionsIL, "Number of Interval Loops With Unknown TripCount");
STATISTIC(NumUnknownConditionsEL, "Number of Equality Loops With Unknown TripCount");
STATISTIC(NumUnknownConditionsOL, "Number of Other Loops With Unknown TripCount");

STATISTIC(NumIncompatibleOperandTypes, "Number of Loop Conditions With non-integer Operands");





using namespace llvm;

//#define enabledebug

#ifdef enabledebug
#define printdebug(X) do { X } while(false)
#else
#define printdebug(X) do { } while(false)
#endif

bool debugMode;

bool llvm::TripCountGenerator::doInitialization(Module& M) {

	context = &M.getContext();

	NumVectorEstimatedTCs = 0;
	NumPericlesEstimatedTCs = 0;

	NumIntervalLoops = 0;
	NumEqualityLoops = 0;
	NumOtherLoops = 0;

	NumUnknownConditionsIL = 0;
	NumUnknownConditionsEL = 0;
	NumUnknownConditionsOL = 0;

	return true;
}

Value* TripCountGenerator::generatePericlesEstimatedTripCount(BasicBlock* header, BasicBlock* entryBlock, Value* Op1, Value* Op2, CmpInst* CI){

	bool isSigned = CI->isSigned();

	BasicBlock* GT = BasicBlock::Create(*context, "", header->getParent(), header);
	BasicBlock* LE = BasicBlock::Create(*context, "", header->getParent(), header);
	BasicBlock* PHI = BasicBlock::Create(*context, "", header->getParent(), header);

	TerminatorInst* T = entryBlock->getTerminator();

	IRBuilder<> Builder(T);


	//Make sure the two operands have the same type
	if (Op1->getType() != Op2->getType()) {

		if (Op1->getType()->getIntegerBitWidth() > Op2->getType()->getIntegerBitWidth() ) {
			//expand op2
			if (isSigned) Op2 = Builder.CreateSExt(Op2, Op1->getType(), "");
			else Op2 = Builder.CreateZExt(Op2, Op1->getType(), "");

		} else {
			//expand op1
			if (isSigned) Op1 = Builder.CreateSExt(Op1, Op2->getType(), "");
			else Op1 = Builder.CreateZExt(Op1, Op2->getType(), "");

		}

	}

	assert(Op1->getType() == Op2->getType() && "Operands with different data types, even after adjust!");


	Value* cmp;

	if (isSigned)
		cmp = Builder.CreateICmpSGT(Op1,Op2,"");
	else
		cmp = Builder.CreateICmpUGT(Op1,Op2,"");

	Builder.CreateCondBr(cmp, GT, LE, NULL);
	T->eraseFromParent();

	/*
	 * estimatedTripCount = |Op1 - Op2|
	 *
	 * We will create the same sub in both GT and in LE blocks, but
	 * with inverted operand order. Thus, the result of the subtraction
	 * will be always positive.
	 */

	Builder.SetInsertPoint(GT);
	Value* sub1;
	if (isSigned) {
		//We create a signed sub
		sub1 = Builder.CreateNSWSub(Op1, Op2);
	} else {
		//We create an unsigned sub
		sub1 = Builder.CreateNUWSub(Op1, Op2);
	}
	Builder.CreateBr(PHI);

	Builder.SetInsertPoint(LE);
	Value* sub2;
	if (isSigned) {
		//We create a signed sub
		sub2 = Builder.CreateNSWSub(Op2, Op1);
	} else {
		//We create an unsigned sub
		sub2 = Builder.CreateNUWSub(Op2, Op1);
	}
	Builder.CreateBr(PHI);

	Builder.SetInsertPoint(PHI);
	PHINode* sub = Builder.CreatePHI(sub2->getType(), 2, "");
	sub->addIncoming(sub1, GT);
	sub->addIncoming(sub2, LE);

	Value* EstimatedTripCount;
	if (isSigned) 	EstimatedTripCount = Builder.CreateSExtOrBitCast(sub, Type::getInt64Ty(*context), "EstimatedTripCount");
	else			EstimatedTripCount = Builder.CreateZExtOrBitCast(sub, Type::getInt64Ty(*context), "EstimatedTripCount");

	switch(CI->getPredicate()){
		case CmpInst::ICMP_UGE:
		case CmpInst::ICMP_ULE:
		case CmpInst::ICMP_SGE:
		case CmpInst::ICMP_SLE:
			{
				Constant* One = ConstantInt::get(EstimatedTripCount->getType(), 1);
				EstimatedTripCount = Builder.CreateAdd(EstimatedTripCount, One);
				break;
			}
		default:
			break;
	}

	//Insert a metadata to identify the instruction as the EstimatedTripCount
	Instruction* i = dyn_cast<Instruction>(EstimatedTripCount);
	MarkAsTripCount(*i);

	Builder.CreateBr(header);

	//Adjust the PHINodes of the loop header accordingly
	for (BasicBlock::iterator it = header->begin(); it != header->end(); it++){
		Instruction* tmp = it;

		if (PHINode* I = dyn_cast<PHINode>(tmp)){
			int i = I->getBasicBlockIndex(entryBlock);
			if (i >= 0){
				I->setIncomingBlock(i,PHI);
			}
		}

	}



	return EstimatedTripCount;
}

Value* TripCountGenerator::getValueAtEntryPoint(Value* source, BasicBlock* loopHeader){

	LoopInfoEx& li = getAnalysis<LoopInfoEx>();
	LoopNormalizerAnalysis& ln = getAnalysis<LoopNormalizerAnalysis>();

	Loop* loop = li.getLoopFor(loopHeader);

	//Option 1: Loop invariant. Return the value itself
	if (loop->isLoopInvariant(source)) {
		return source;
	}

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

template <typename T>
std::stack<T> invertStack(std::stack<T> input){

	std::stack<T> result;

	while (input.size() > 0) {
		result.push(input.top());
		input.pop();
	}

	return result;
}


ProgressVector* TripCountGenerator::joinVectors(ProgressVector* Vec1, ProgressVector* Vec2){

	/*
	 * TODO: join variable vectors >>> Symbolic RA
	 */


	Value* Val1 = Vec1->getUniqueValue(Type::getInt64Ty(*context));
	Value* Val2 = Vec2->getUniqueValue(Type::getInt64Ty(*context));

	ConstantInt* C1 = dyn_cast<ConstantInt>(Val1);
	ConstantInt* C2 = dyn_cast<ConstantInt>(Val2);

	if ((!C1) || (!C2)) return NULL;


	int64_t V1  = C1->getValue().getSExtValue();
	int64_t V2  = C2->getValue().getSExtValue();

	// Both vectors must have the same direction
	if ( (V1 > 0 && V2 < 0) || (V2 > 0 && V1 < 0) ) return NULL;


	if (V1 > 0 || V2 > 0) {

		//estimateMinimumTripCount >> get the vector with greatest absolute size
		if (estimateMinimumTripCount) {
			return (V1 > V2 ? Vec1 : Vec2);
		} else {
			return (V1 < V2 ? Vec1 : Vec2);
		}

	} else {

		//estimateMinimumTripCount >> get the vector with greatest absolute size
		if (estimateMinimumTripCount) {
			return (V1 < V2 ? Vec1 : Vec2);
		} else {
			return (V1 > V2 ? Vec1 : Vec2);
		}

	}

}


ProgressVector* TripCountGenerator::generateConstantProgressVector(Value* source, BasicBlock* loopHeader){


	LoopInfoEx & li = getAnalysis<LoopInfoEx>();
	LoopControllersDepGraph & lcd = getAnalysis<LoopControllersDepGraph>();
	DepGraph* depGraph = lcd.depGraph;
	depGraph->recomputeSCCs();

	GraphNode* sourceNode = depGraph->findNode(source);

	ProgressVector* result = NULL;


	std::set<std::stack<GraphNode*> > paths = depGraph->getAcyclicPathsInsideSCC(sourceNode, sourceNode);

	//When a value has no cycle (single-node SCC) we must create an artificial cycle that
	//produces a vector with length zero
	if (!paths.size()) {
		std::stack<GraphNode*> forcedPath;
		forcedPath.push(sourceNode);
		forcedPath.push(sourceNode);
		paths.insert(forcedPath);
	}

	//We will not analyze values with more than 999 paths of redefinitions
	if (paths.size() > 999) return result;

	//We will now evaluate each of the paths and aggregate them according to the
	//argument VectorAggregation
	for (std::set<std::stack<GraphNode*> >::iterator pIt = paths.begin(); pIt != paths.end(); pIt++){

		std::stack<GraphNode*> inversePath = *pIt;

		if (inversePath.top() != sourceNode) inversePath.push(sourceNode);

		//Here we extract the sequence of values in this path
		std::set<Value*> uniqueValues;
		std::list<Value*> path;

		while (inversePath.size() > 0) {
			Value* v = NULL;

			if ( VarNode* VN = dyn_cast<VarNode>( inversePath.top() )  ) {
				v = VN->getValue();
			}

			if ( OpNode* ON = dyn_cast<OpNode>( inversePath.top() )  ) {
				v = ON->getOperation();
			}

			if (v) {

				if (!uniqueValues.count(v)) {
					uniqueValues.insert(v);
					path.push_front(v);
				}

			}

			inversePath.pop();

		}

		//First and last node must be the same
		path.push_front(path.back());


		//Here we generate the vector for this specific path
		ProgressVector* currentPathVector = new ProgressVector(path);
		Value* currentVectorValue = currentPathVector->getUniqueValue(Type::getInt64Ty(*context));


		bool fail = false;

		if (currentVectorValue) {

			if (currentVectorValue->getName().equals("add97")){
				errs() << "Achei!\n";

				errs() << "Source: " << *source << "\n";
				if(Instruction* I = dyn_cast<Instruction>(source)){
					errs() << *(I->getParent())  << "\n\n";
				}


				errs() << *loopHeader;


			}


			if (li.getLoopFor(loopHeader)->isLoopInvariant(currentVectorValue)) {

				if (!result) result = currentPathVector;
				else {
					ProgressVector* tmp = joinVectors(result, currentPathVector);

					if (!tmp) {
						fail = true;
					}
					else {
						if (tmp == result) delete currentPathVector;
						else delete result;

						result = tmp;
					}
				}

			} else {
				fail = true;
			}


		} else {
			fail = true;
		}


		if (fail) {
			delete currentPathVector;
			if (result) delete result;
			return NULL;
		}

	}

	return result;
}

void equalizeTypes(Value* &Op1, Value* &Op2, bool isSigned, IRBuilder<> Builder){

	if (Op1->getType() != Op2->getType()) {

		if (Op1->getType()->getIntegerBitWidth() > Op2->getType()->getIntegerBitWidth() ) {
			//expand op2
			if (isSigned) Op2 = Builder.CreateSExt(Op2, Op1->getType(), "");
			else Op2 = Builder.CreateZExt(Op2, Op1->getType(), "");

		} else {
			//expand op1
			if (isSigned) Op1 = Builder.CreateSExt(Op1, Op2->getType(), "");
			else Op1 = Builder.CreateZExt(Op1, Op2->getType(), "");

		}

	}

}


Instruction* TripCountGenerator::generateReplaceIfEqual
		(Value* Op, Value* ValueToTest, Value* ValueToReplace,
				Instruction* InsertBefore){

	BasicBlock* startBB = InsertBefore->getParent();
	BasicBlock* endBB = InsertBefore->getParent()->splitBasicBlock(InsertBefore);

	TerminatorInst* T = startBB->getTerminator();

	IRBuilder<> Builder(T);

	BasicBlock* EQ = BasicBlock::Create(*context, "", InsertBefore->getParent()->getParent(), endBB);

	Value* cmp;
	cmp = Builder.CreateICmpEQ(Op,ValueToTest,"");
	Builder.CreateCondBr(cmp, EQ, endBB, NULL);
	T->eraseFromParent();

	Builder.SetInsertPoint(EQ);
	Builder.CreateBr(endBB);

	Builder.SetInsertPoint(InsertBefore);
	PHINode* phi = Builder.CreatePHI(Op->getType(), 2, "");
	phi->addIncoming(ValueToReplace, EQ);
	phi->addIncoming(Op, startBB);

	return phi;

}

Instruction* llvm::TripCountGenerator::generateModuleOfSubtraction
		(Value* Op1, Value* Op2, bool isSigned, Instruction* InsertBefore){

	BasicBlock* startBB = InsertBefore->getParent();
	BasicBlock* endBB = InsertBefore->getParent()->splitBasicBlock(InsertBefore);

	TerminatorInst* T = startBB->getTerminator();

	IRBuilder<> Builder(T);

	//Make sure the two operands have the same type
	equalizeTypes(Op1, Op2, isSigned, Builder);
	assert(Op1->getType() == Op2->getType() && "Operands with different data types, even after adjust!");


	BasicBlock* GT = BasicBlock::Create(*context, "", InsertBefore->getParent()->getParent(), endBB);
	BasicBlock* LE = BasicBlock::Create(*context, "", InsertBefore->getParent()->getParent(), endBB);

	Value* cmp;

	if (isSigned)
		cmp = Builder.CreateICmpSGT(Op1,Op2,"");
	else
		cmp = Builder.CreateICmpUGT(Op1,Op2,"");

	Builder.CreateCondBr(cmp, GT, LE, NULL);
	T->eraseFromParent();

	/*
	 * ModuleOfSubtraction = |Op1 - Op2|
	 *
	 * We will create the same sub in both GT and in LE blocks, but
	 * with inverted operand order. Thus, the result of the subtraction
	 * will be always positive.
	 */

	Builder.SetInsertPoint(GT);
	Value* sub1;
	if (isSigned) {
		//We create a signed sub
		sub1 = Builder.CreateNSWSub(Op1, Op2, "diff");
	} else {
		//We create an unsigned sub
		sub1 = Builder.CreateNUWSub(Op1, Op2, "diff");
	}
	Builder.CreateBr(endBB);


	Builder.SetInsertPoint(LE);
	Value* sub2;
	if (isSigned) {
		//We create a signed sub
		sub2 = Builder.CreateNSWSub(Op2, Op1, "diff");
	} else {
		//We create an unsigned sub
		sub2 = Builder.CreateNUWSub(Op2, Op1, "diff");
	}
	Builder.CreateBr(endBB);

	Builder.SetInsertPoint(InsertBefore);
	PHINode* sub = Builder.CreatePHI(sub2->getType(), 2, "");
	sub->addIncoming(sub1, GT);
	sub->addIncoming(sub2, LE);

	return sub;
}


Value* llvm::TripCountGenerator::generateVectorEstimatedTripCount(
		BasicBlock* header, BasicBlock* entryBlock, Value* Op1, Value* Op2,
		ProgressVector* V1, ProgressVector* V2, ICmpInst* CI) {

	Value* _V1 = V1->getUniqueValue(Op1->getType());
	Value* _V2 = V2->getUniqueValue(Op1->getType());

	if(!_V1 || !_V2) return NULL;

	TerminatorInst* T = entryBlock->getTerminator();
	bool isSigned = CI->isSigned();

	Instruction* initialDistance = generateModuleOfSubtraction(Op1, Op2, isSigned, T);

	Instruction* step = generateModuleOfSubtraction(_V1, _V2, isSigned, T);

	//Preventing division by zero
	step = generateReplaceIfEqual(step, ConstantInt::get(step->getType(),0), ConstantInt::get(step->getType(),1), T);

	IRBuilder<> Builder(T);

	Instruction* EstimatedTripCount;
	if (isSigned) {
		//We create a signed div
		EstimatedTripCount = BinaryOperator::CreateSDiv(initialDistance, step, "TC", T);
	} else {
		//We create an unsigned div
		EstimatedTripCount = BinaryOperator::CreateUDiv(initialDistance, step, "TC", T);
	}

	if (isSigned) 	EstimatedTripCount = CastInst::CreateSExtOrBitCast(EstimatedTripCount, Type::getInt64Ty(*context), "EstimatedTripCount", T);
	else			EstimatedTripCount = CastInst::CreateSExtOrBitCast(EstimatedTripCount, Type::getInt64Ty(*context), "EstimatedTripCount", T);


	switch(CI->getPredicate()){
		case CmpInst::ICMP_UGE:
		case CmpInst::ICMP_ULE:
		case CmpInst::ICMP_SGE:
		case CmpInst::ICMP_SLE:
			{
				Constant* One = ConstantInt::get(EstimatedTripCount->getType(), 1);
				EstimatedTripCount = BinaryOperator::CreateAdd(EstimatedTripCount, One, "", T);
				break;
			}
		default:
			break;
	}

	//Insert a metadata to identify the instruction as the EstimatedTripCount
	MarkAsTripCount(*EstimatedTripCount);

	//Adjust the PHINodes of the loop header accordingly
	//
	//This is necessary because one of the incoming blocks of the loop header has changed
	for (BasicBlock::iterator it = header->begin(); it != header->end(); it++){
		Instruction* tmp = it;

		if (PHINode* I = dyn_cast<PHINode>(tmp)){
			int i = I->getBasicBlockIndex(entryBlock);
			if (i >= 0){
				I->setIncomingBlock(i,EstimatedTripCount->getParent());
			}
		}

	}

	return EstimatedTripCount;
}

void llvm::TripCountGenerator::generateHybridEstimatedTripCounts(Function& F) {

	LoopInfoEx& li = getAnalysis<LoopInfoEx>();
	LoopNormalizerAnalysis& ln = getAnalysis<LoopNormalizerAnalysis>();

	for(LoopInfoEx::iterator lit = li.begin(); lit != li.end(); lit++){



		//Indicates if we don't have ways to determine the trip count
		bool unknownTC = false;

		Loop* loop = *lit;

		BasicBlock* header = loop->getHeader();
		BasicBlock* entryBlock = ln.entryBlocks[header];


		LoopControllersDepGraph& lcd = getAnalysis<LoopControllersDepGraph>();
		lcd.setPerspective(header);

		/*
		 * Here we are looking for the predicate that stops the loop.
		 *
		 * At this moment, we are only considering loops that are controlled by
		 * integer comparisons.
		 */
		BasicBlock* exitBlock = findLoopControllerBlock(loop);
		assert(exitBlock && "Exiting Block not found!");

		TerminatorInst* T = exitBlock->getTerminator();
		BranchInst* BI = dyn_cast<BranchInst>(T);
		ICmpInst* CI = BI ? dyn_cast<ICmpInst>(BI->getCondition()) : NULL;

		Value* Op1 = NULL;
		Value* Op2 = NULL;

		if (!CI) unknownTC = true;
		else {

			int LoopClass;
			if (isIntervalComparison(CI)) {
				LoopClass = 0;
				NumIntervalLoops++;
			} else {
				LoopClass = 1;
				NumEqualityLoops++;
			}

			Op1 = getValueAtEntryPoint(CI->getOperand(0), header);
			Op2 = getValueAtEntryPoint(CI->getOperand(1), header);




			if((!Op1) || (!Op2) ) {

				if (!LoopClass) NumUnknownConditionsIL++;
				else 			NumUnknownConditionsEL++;

				unknownTC = true;
			} else {

				if (!(Op1->getType()->isIntegerTy() && Op2->getType()->isIntegerTy())) {
					//We only handle loop conditions that compares integer variables
					NumIncompatibleOperandTypes++;
					unknownTC = true;
				}

			}

		}

		ProgressVector* V1 = NULL;
		ProgressVector* V2 = NULL;



		if (!unknownTC) {
			V1 = generateConstantProgressVector(CI->getOperand(0), header);
			V2 = generateConstantProgressVector(CI->getOperand(1), header);

			//No vectors available? Try Pericles instead!
			if ((!V1) || (!V2)) {

				generatePericlesEstimatedTripCount(header, entryBlock, Op1, Op2, CI);
				NumPericlesEstimatedTCs++;

				//This will avoid  calling generateVectorEstimatedTripCount
				unknownTC = true;
			}

		}

		if(!unknownTC) {
			generateVectorEstimatedTripCount(header, entryBlock, Op1, Op2, V1, V2, CI);
			NumVectorEstimatedTCs++;
		}


	}

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
BasicBlock* TripCountGenerator::findLoopControllerBlock(Loop* l){

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

		//Here we iterate over the successors of BB to check if the header is one of them
		for(succ_iterator s = succ_begin(BB); s != succ_end(BB); s++){

			if (*s == header) {
				return BB;
			}
		}

	}

	//Otherwise, return the first exit block
	return *(exitBlocks.begin());
}

bool TripCountGenerator::isIntervalComparison(ICmpInst* CI){

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

void TripCountGenerator::generatePericlesEstimatedTripCounts(Function &F){

	LoopInfoEx& li = getAnalysis<LoopInfoEx>();
	LoopNormalizerAnalysis& ln = getAnalysis<LoopNormalizerAnalysis>();

	for(LoopInfoEx::iterator lit = li.begin(); lit != li.end(); lit++){

		//Indicates if we don't have ways to determine the trip count
		bool unknownTC = false;

		Loop* loop = *lit;

		BasicBlock* header = loop->getHeader();
		BasicBlock* entryBlock = ln.entryBlocks[header];

		LoopControllersDepGraph& lcd = getAnalysis<LoopControllersDepGraph>();
		lcd.setPerspective(header);

		/*
		 * Here we are looking for the predicate that stops the loop.
		 *
		 * At this moment, we are only considering loops that are controlled by
		 * integer comparisons.
		 */
		BasicBlock* exitBlock = findLoopControllerBlock(loop);
		assert(exitBlock && "ExitBlock not found!");

		TerminatorInst* T = exitBlock->getTerminator();
		BranchInst* BI = dyn_cast<BranchInst>(T);
		ICmpInst* CI = BI ? dyn_cast<ICmpInst>(BI->getCondition()) : NULL;

		Value* Op1 = NULL;
		Value* Op2 = NULL;

		int LoopClass = 0;

		if (!CI) {
			unknownTC = true;
			NumOtherLoops++;
			NumUnknownConditionsOL++;

			LoopClass = 2;
		}
		else {

			if (isIntervalComparison(CI)) {
				LoopClass = 0;
				NumIntervalLoops++;
			} else {
				LoopClass = 1;
				NumEqualityLoops++;
			}

			Op1 = getValueAtEntryPoint(CI->getOperand(0), header);
			Op2 = getValueAtEntryPoint(CI->getOperand(1), header);


			if((!Op1) || (!Op2) ) {

				if (!LoopClass) NumUnknownConditionsIL++;
				else 			NumUnknownConditionsEL++;

				unknownTC = true;
			} else {


				if (!(Op1->getType()->isIntegerTy() && Op2->getType()->isIntegerTy())) {

					/*
					 * We only handle integer loop conditions
					 */
					NumIncompatibleOperandTypes++;
					unknownTC = true;
				}

			}

		}

		if(!unknownTC) {
			generatePericlesEstimatedTripCount(header, entryBlock, Op1, Op2, CI);
			NumPericlesEstimatedTCs++;
		}


	}

}


void TripCountGenerator::generateVectorEstimatedTripCounts(Function &F){

	LoopInfoEx& li = getAnalysis<LoopInfoEx>();
	LoopNormalizerAnalysis& ln = getAnalysis<LoopNormalizerAnalysis>();

	for(LoopInfoEx::iterator lit = li.begin(); lit != li.end(); lit++){

		//Indicates if we don't have ways to determine the trip count
		bool unknownTC = false;

		Loop* loop = *lit;

		BasicBlock* header = loop->getHeader();
		BasicBlock* entryBlock = ln.entryBlocks[header];

		LoopControllersDepGraph& lcd = getAnalysis<LoopControllersDepGraph>();
		lcd.setPerspective(header);

		/*
		 * Here we are looking for the predicate that stops the loop.
		 *
		 * At this moment, we are only considering loops that are controlled by
		 * integer comparisons.
		 */
		BasicBlock* exitBlock = findLoopControllerBlock(loop);
		assert(exitBlock && "Exiting Block not found!");

		TerminatorInst* T = exitBlock->getTerminator();
		BranchInst* BI = dyn_cast<BranchInst>(T);
		ICmpInst* CI = BI ? dyn_cast<ICmpInst>(BI->getCondition()) : NULL;

		Value* Op1 = NULL;
		Value* Op2 = NULL;

		if (!CI) unknownTC = true;
		else {

			int LoopClass;
			if (isIntervalComparison(CI)) {
				LoopClass = 0;
				NumIntervalLoops++;
			} else {
				LoopClass = 1;
				NumEqualityLoops++;
			}

			Op1 = getValueAtEntryPoint(CI->getOperand(0), header);
			Op2 = getValueAtEntryPoint(CI->getOperand(1), header);


			if((!Op1) || (!Op2) ) {

				if (!LoopClass) NumUnknownConditionsIL++;
				else 			NumUnknownConditionsEL++;

				unknownTC = true;
			} else {


				if (!(Op1->getType()->isIntegerTy() && Op2->getType()->isIntegerTy())) {
					//We only handle loop conditions that compares integer variables
					NumIncompatibleOperandTypes++;
					unknownTC = true;
				}

			}

		}

		ProgressVector* V1 = NULL;
		ProgressVector* V2 = NULL;


		if (!unknownTC) {
			V1 = generateConstantProgressVector(CI->getOperand(0), header);
			V2 = generateConstantProgressVector(CI->getOperand(1), header);

			if ((!V1) || (!V2)) {

				//TODO: Increment a statistic here
				unknownTC = true;
			}

		}

		if(!unknownTC) {
			generateVectorEstimatedTripCount(header, entryBlock, Op1, Op2, V1, V2, CI);
			NumVectorEstimatedTCs++;
		}


	}

}

bool TripCountGenerator::runOnFunction(Function &F){

	if (usePericlesTripCount)
		generatePericlesEstimatedTripCounts(F);
	else if (useHybridTripCount)
		generateHybridEstimatedTripCounts(F);
	else
		generateVectorEstimatedTripCounts(F);

	return true;
}

void TripCountGenerator::MarkAsTripCount(Instruction& inst)
{
	std::vector<Value*> vec;
	ArrayRef<Value*> aref(vec);
    inst.setMetadata("TripCount", MDNode::get(*context, aref));
}

bool TripCountGenerator::IsTripCount(Instruction& inst)
{
    return inst.getMetadata("TripCount") != 0;
}

char llvm::TripCountGenerator::ID = 0;
static RegisterPass<TripCountGenerator> X("tc-generator","Trip Count Generator");
