/*
 * RangeAnalysis.cpp
 *
 *  Created on: Mar 19, 2014
 *      Author: raphael
 */

#define DEBUG_TYPE "range-analysis"

#include "RangeAnalysis.h"

using namespace std;

//Bitwidth reduction statistics
STATISTIC(usedBits, "Initial number of bits.");
STATISTIC(needBits, "Needed bits.");
STATISTIC(percentReduction, "Percentage of reduction of the number of bits.");

//SCC statistics
STATISTIC(numSCCs, "Number of strongly connected components.");
STATISTIC(numAloneSCCs, "Number of SCCs containing only one node.");
STATISTIC(sizeMaxSCC, "Size of largest SCC.");

//Graph nodes statistics
STATISTIC(numVars, "Number of variables");
STATISTIC(numOps, "Number of operations");
STATISTIC(numMems, "Number of memory nodes");

//Range statistics
STATISTIC(numUnknown, "Number of unknown ranges");
STATISTIC(numEmpty, "Number of empty ranges");
STATISTIC(numCPlusInf, "Number of [c, +inf].");
STATISTIC(numCC, "Number of [c, c].");
STATISTIC(numMinInfC, "Number of [-inf, c].");
STATISTIC(numMaxRange, "Number of [-inf, +inf].");
STATISTIC(numConstants, "Number of constants.");



cl::opt<std::string> RAFilename("ra-filename",
		                          cl::desc("Specify pre-computed ranges filename"),
		                          cl::value_desc("filename"),
		                          cl::init("stdin"),
		                          cl::NotHidden);

cl::opt<std::string> RAIgnoredFunctions("ra-ignore-functions",
		                               cl::desc("Specify file with functions to be ignored"),
		                               cl::value_desc("filename"),
		                               cl::init(""),
		                               cl::NotHidden);



/*
 * method solve
 *
 * Solves the range analysis. At this point, the dependence graph is already
 * computed and the constraints have already been extracted.
 * In addition, the control dependence edges have already been added to the graph.
 */
void llvm::RangeAnalysis::solve() {

	depGraph->recomputeSCCs();
	std::list<int> SCCorder = depGraph->getSCCTopologicalOrder();

	//Statistic: Number of SCCs
	numSCCs += SCCorder.size();

	//Iterate over the SCCs of the dependence graph in topological order
	for(std::list<int>::iterator SCCit = SCCorder.begin(), SCCend = SCCorder.end(); SCCit != SCCend; SCCit++ ){

		int SCCid = *SCCit;

		//Growth Analysis
		fixPointIteration(SCCid, loJoin);

		//Future Resolution
		fixFutures(SCCid);

		//Narrowing Analysis
		fixPointIteration(SCCid, loMeet);
	}

	computeStats();

}

/*
 * function evaluateNode
 *
 * Given a node, this function computes the abstract state of the node
 * considering the abstract state of the node's predecessors and the
 * semantics of the node.
 */
Range llvm::RangeAnalysis::evaluateNode(GraphNode* Node){

	Range result;

	/*
	 * VarNode: Constants generate constant ranges;
	 * otherwise, the output is a union of the predecessors
	 * */
	if(VarNode* VN = dyn_cast<VarNode>(Node)){
		Value* val = VN->getValue();

		if (ConstantInt* CI = dyn_cast<ConstantInt>(val)){
			APInt value = CI->getValue();
			value = value.sextOrTrunc(MAX_BIT_INT);
			result = Range(value,value);
		} else {
			result = getUnionOfPredecessors(Node);
		}

	}
	/*
	 * Nodes that do not modify the data: PHI, Sigma, MemNode
	 * >> Just forward the state to the next node.
	 */
	else if (isa<MemNode>(Node) || isa<SigmaOpNode>(Node) || isa<PHIOpNode>(Node) || ( isa<OpNode>(Node) && dyn_cast<OpNode>(Node)->getOpCode() == Instruction::PHI ) ) {

		result = getUnionOfPredecessors(Node);
	}
	/*
	 * CallNodes: We do not know the output
	 * >> [-inf, +inf]
	 */
	else if (isa<CallNode>(Node)) {

		result = Range(Min, Max);
	}
	/*
	 * Binary operators: we will do the abstract interpretation
	 * to generate the result
	 */
	else if (BinaryOpNode* BOP = dyn_cast<BinaryOpNode>(Node)) {

		GraphNode* Op1 = BOP->getOperand(0);
		GraphNode* Op2 = BOP->getOperand(1);

		result = abstractInterpretation(out_state[Op1], out_state[Op2], BOP->getBinaryOperator());
	}
	/*
	 * Unary operators: we will do the abstract interpretation
	 * to generate the result
	 */
	else if (UnaryOpNode* UOP = dyn_cast<UnaryOpNode>(Node)) {

		GraphNode* Op = UOP->getIncomingNode(0);

		result = abstractInterpretation(out_state[Op], UOP->getUnaryInstruction());
	}
	/*
	 * Generic operations: treat individually case by case
	 */
	else if (OpNode* OP = dyn_cast<OpNode>(Node)) {

		if (Instruction* I = OP->getOperation()) {

			switch (I->getOpcode()) {
			case Instruction::Store:
				GraphNode* Op = OP->getIncomingNode(0);
				result = abstractInterpretation(out_state[Op], I);
				break;
			}
		}

	} else {
		result = Range();
	}

	return result;
}


/*
 * function getUnionOfPredecessors
 *
 * Returns the union of the abstract states of the
 * predecessors of the node passed as the parameter.
 */
Range llvm::RangeAnalysis::getUnionOfPredecessors(GraphNode* Node){
	Range result(Min, Max, Unknown);

	std::map<GraphNode*, edgeType> Preds = Node->getPredecessors();
	std::map<GraphNode*, edgeType>::iterator pred, pred_end;
	for(pred = Preds.begin(), pred_end = Preds.end(); pred != pred_end; pred++){

		if(pred->second == etData){

			//Ignore the data that comes from ignored functions
			if (CallNode* CI = dyn_cast<CallNode>(pred->first)){
				Function* F = CI->getCalledFunction();
				if (ignoredFunctions.count(F->getName())) continue;
			}


			result = result.unionWith(out_state[pred->first]);
		}
	}

	return result;
}

/*
 * Abstract interpretation of binary operators
 */
Range llvm::RangeAnalysis::abstractInterpretation(Range Op1, Range Op2, Instruction *I){

	if (Op1.isUnknown() || Op2.isUnknown()) {
		return Range(Min, Max, Unknown);
	}

	switch(I->getOpcode()){
		case Instruction::Add:  return Op1.add(Op2);
		case Instruction::Sub:  return Op1.sub(Op2);
		case Instruction::Mul:  return Op1.mul(Op2);
		case Instruction::SDiv: return Op1.sdiv(Op2);
		case Instruction::UDiv: return Op1.udiv(Op2);
		case Instruction::SRem: return Op1.srem(Op2);
		case Instruction::URem: return Op1.urem(Op2);
		case Instruction::Shl:  return Op1.shl(Op2);
		case Instruction::AShr: return Op1.ashr(Op2);
		case Instruction::LShr: return Op1.lshr(Op2);
		case Instruction::And:  return Op1.And(Op2);
		case Instruction::Or:   return Op1.Or(Op2);
		case Instruction::Xor:  return Op1.Xor(Op2);
		default:
			errs() << "Unhandled Instruction:" << *I;
			return Range(Min,Max);
	}
}

/*
 * Abstract interpretation of unary operators
 */
Range llvm::RangeAnalysis::abstractInterpretation(Range Op1, Instruction *I){

	unsigned bw = I->getType()->getPrimitiveSizeInBits();
	Range result(Min, Max, Unknown);

	if (Op1.isRegular()) {
		switch (I->getOpcode()) {
		case Instruction::Trunc:
			result = Op1.truncate(bw);
			break;
		case Instruction::ZExt:
			result = Op1.zextOrTrunc(bw);
			break;
		case Instruction::SExt:
			result = Op1.sextOrTrunc(bw);
			break;
		case Instruction::Load:
			result = Op1;
			break;
		case Instruction::Store:
			result = Op1;
			break;
		case Instruction::BitCast: {
			result = Op1;
			break;
		}
		default:
			errs() << "Unhandled UnaryInstruction:" << *I;
			result = Range(Min, Max);
			break;
		}
	} else if (Op1.isEmpty()) {
		result = Range(Min, Max, Empty);
	}

	return result;
}


/*
 * Here we do the abstract interpretation.
 *
 * We compute the out_state of a node based in the out_state of
 * the predecessors of the node.
 *
 * We also perform the widening operation as needed.
 *
 * Finally, if the out_state is changed, we add to the worklist the successor nodes.
 */
void llvm::RangeAnalysis::computeNode(GraphNode* Node, std::set<GraphNode*> &Worklist, LatticeOperation lo){

	Range new_out_state = evaluateNode(Node);

	bool hasChanged = (lo == loJoin ? join(Node, new_out_state) : meet(Node, new_out_state));

	if (hasChanged) {
		//The range of this node has changed. Add its successors to the worklist.
		addSuccessorsToWorklist(Node, Worklist);
	}

}

/*
 * method addSuccessorsToWorklist
 *
 * Insert the successors of a node in the worklist
 */
void llvm::RangeAnalysis::addSuccessorsToWorklist(GraphNode* Node, std::set<GraphNode*> &Worklist){

	int SCCid = depGraph->getSCCID(Node);

	std::map<GraphNode*, edgeType> succs = Node->getSuccessors();

	std::map<GraphNode*, edgeType>::iterator succ, succ_end;
	for(succ = succs.begin(), succ_end = succs.end(); succ != succ_end; succ++){

		if(depGraph->getSCCID(succ->first) == SCCid  && succ->second == etData){
			Worklist.insert(succ->first);
		}
	}

}

/*
 * function join
 *
 * Here we perform the join operation in the interval lattice,
 * using Cousot's widening operator. We join the current abstract state
 * with the new_abstract_state and update the current abstract state of
 * the node.
 *
 * This function returns true if the current abstract state has changed.
 */
bool llvm::RangeAnalysis::join(GraphNode* Node, Range new_abstract_state){



	Range oldInterval = out_state[Node];
	Range newInterval = new_abstract_state;

	APInt oldLower = oldInterval.getLower();
	APInt oldUpper = oldInterval.getUpper();
	APInt newLower = newInterval.getLower();
	APInt newUpper = newInterval.getUpper();



	if (oldInterval.isUnknown())
		out_state[Node] = newInterval;
	else {

		if (widening_count[Node] < MaxIterationCount) {
			out_state[Node] = oldInterval.unionWith(newInterval);
			widening_count[Node]++;
		} else {
			//Widening
			APInt oldLower = oldInterval.getLower();
			APInt oldUpper = oldInterval.getUpper();
			APInt newLower = newInterval.getLower();
			APInt newUpper = newInterval.getUpper();
			if (newLower.slt(oldLower))
				if (newUpper.sgt(oldUpper))
					out_state[Node] = Range(Min, Max);
				else
					out_state[Node] = Range(Min, oldUpper);
			else if (newUpper.sgt(oldUpper))
				out_state[Node] = Range(oldLower, Max);
		}
	}

	bool hasChanged = oldInterval != out_state[Node];

	return hasChanged;
}


/*
 * function meet
 *
 * Here we perform the meet operation in the interval lattice,
 * using Cousot's narrowing operator. We meet the current abstract state
 * with the new_abstract_state and update the current abstract state of
 * the node.
 *
 * This function returns true if the current abstract state has changed.
 */
bool llvm::RangeAnalysis::meet(GraphNode* Node, Range new_abstract_state){

	Range oldInterval = out_state[Node];
	APInt oLower = out_state[Node].getLower();
	APInt oUpper = out_state[Node].getUpper();
	Range newInterval = new_abstract_state;

	APInt nLower = newInterval.getLower();
	APInt nUpper = newInterval.getUpper();

	if (narrowing_count[Node] < MaxIterationCount) {

		if (oLower.eq(Min) && nLower.ne(Min)) {
			out_state[Node] = Range(nLower, oUpper);
		} else {
			APInt smin = APIntOps::smin(oLower, nLower);
			if (oLower.ne(smin)) {
				out_state[Node] = Range(smin, oUpper);
			}
		}

		if (oUpper.eq(Max) && nUpper.ne(Max)) {
			out_state[Node] = Range(out_state[Node].getLower(), nUpper);
		} else {
			APInt smax = APIntOps::smax(oUpper, nUpper);
			if (oUpper.ne(smax)) {
				out_state[Node] = Range(out_state[Node].getLower(), smax);
			}
		}

	}

	if (SigmaOpNode* Sigma = dyn_cast<SigmaOpNode>(Node)){

		if (branchConstraints.count(Sigma) > 0) {
			out_state[Node] = out_state[Node].intersectWith(branchConstraints[Sigma]->getRange());
		}
	}

	bool hasChanged = oldInterval != out_state[Node];

	if (hasChanged) narrowing_count[Node]++;

	return hasChanged;
}


/*
 * method fixFutures
 *
 * Here we replace the future values of the Sigma's constraints with
 * actual values.
 */
void llvm::RangeAnalysis::fixFutures(int SCCid) {

	SCC_Iterator it(depGraph, SCCid);

	while(it.hasNext()){

		if (SigmaOpNode* Node = dyn_cast<SigmaOpNode>(it.getNext())) {

			if (branchConstraints.count(Node) > 0) {

				if (SymbInterval* SI = dyn_cast<SymbInterval>(branchConstraints[Node])) {

					GraphNode* ControlDep = Node->getIncomingNode(0, etControl);
					SI->fixIntersects(out_state[ControlDep]);

				}
			}
		}
	}
}

/*
 * method addConstraints
 *
 * Here we take a set of constraints extracted from the program's
 * CFG and add them to the Dependence Graph.
 *
 * We also insert control dependence edges as needed.
 */
void llvm::RangeAnalysis::addConstraints(
		std::map<const Value*, std::list<ValueSwitchMap*> > constraints) {

	//First we iterate through the constraints
	std::map<const Value*, std::list<ValueSwitchMap*> >::iterator Cit, Cend;
	for(Cit = constraints.begin(), Cend = constraints.end(); Cit != Cend; Cit++){

		const Value* CurrentValue = Cit->first;

		// For each value, we iterate through its uses, looking for sigmas. We
		// can only learn with conditionals when we insert sigmas, because they split the
		// live ranges of the variables according to the branch results.

		Value::const_use_iterator Uit, Uend;
		for(Uit = CurrentValue->use_begin(), Uend = CurrentValue->use_end(); Uit != Uend; Uit++){

			const User* U = *Uit;
			if (!U) continue;

			if (const PHINode* CurrentUse = dyn_cast<const PHINode>(U)){

				if(SigmaOpNode* CurrentSigmaOpNode = dyn_cast<SigmaOpNode>(depGraph->findOpNode(CurrentUse))){

					const BasicBlock* ParentBB = CurrentUse->getParent();

					// We will look for the symbolic range of the basic block of this sigma node.
					std::list<ValueSwitchMap*>::iterator VSMit, VSMend;
					for(VSMit = Cit->second.begin(), VSMend = Cit->second.end(); VSMit != VSMend; VSMit++){

						ValueSwitchMap* CurrentVSM = *VSMit;
						int Idx = CurrentVSM->getBBid(ParentBB);
						if(Idx >= 0){
							//Save the symbolic interval of the opNode. We will use it in the narrowing
							// phase of the range analysis

							BasicInterval* BI = CurrentVSM->getItv(Idx);
							branchConstraints[CurrentSigmaOpNode] = BI;

							//If it is a symbolic interval, then we have a future value and we must
							//insert a control dependence edge in the graph.
							if(SymbInterval* SI = dyn_cast<SymbInterval>(BI)){

								if (GraphNode* FutureValue = depGraph->findNode(SI->getBound())) {
									depGraph->addEdge(FutureValue, CurrentSigmaOpNode, etControl);
								}
							}
							break;
						}
					}
				}
			}
		}
	}
}

/*
 * method importInitialStates
 *
 * Here we read the file specified by RAFilename and initialize the
 * abstract states of the selected nodes in the graph.
 */
void llvm::RangeAnalysis::importInitialStates(ModuleLookup& M){

	if(RAFilename.empty()) return;

	ifstream File;
	File.open(RAFilename.c_str(), ifstream::in);

	std::string line;
	while (std::getline(File, line))
	{
		std::size_t tam = line.size();
		std::size_t pos = line.find_first_of("|");
		std::string FunctionName = line.substr(0, pos);

		line = line.substr(pos+1, tam-pos-1);
		tam = line.size();
		pos = line.find_first_of("|");
		std::string ValueName = line.substr(0, pos);

		std::string RangeString = line.substr(pos+1, tam-pos);

		if(Value* V = M.getValueByName(FunctionName, ValueName)) {

			if(GraphNode* G = depGraph->findNode(V)){
				initial_state[G] = Range(RangeString);
			}
		}
	}

	File.close();
}

/*
 * method loadIgnoredFunctions
 *
 * Here we read the file specified by FileName and build a set of
 * functions that will not propagate abstract states to other nodes
 * of the graph.
 */
void llvm::RangeAnalysis::loadIgnoredFunctions(std::string FileName){

	if (FileName.empty()) return;

	ifstream File;
	File.open(FileName.c_str(), ifstream::in);

	while(!File.eof()){

		std::string FunctionName;
		File >> FunctionName;

		addIgnoredFunction(FunctionName);
	}

	File.close();
}

/*
 * method addIgnoredFunction
 *
 * Here we add to the list the name of the function that we want to ignore.
 */
void llvm::RangeAnalysis::addIgnoredFunction(std::string FunctionName){

	if(!ignoredFunctions.count(FunctionName)) ignoredFunctions.insert(FunctionName);

}

/*
 * method computeStats
 *
 * computes the statistics that require the processing to be complete.
 */
void llvm::RangeAnalysis::computeStats(){

	for(DepGraph::iterator It = depGraph->begin(), Iend = depGraph->end(); It != Iend; It++){

		GraphNode* Node = *It;

		bool isIntegerVariable = false;
		unsigned int currentBitWidth;

		//We only count precision statistics of VarNodes and MemNodes
		if (isa<OpNode>(Node)) {
			numOps++;
			continue;
		}

		if (isa<MemNode>(Node)) {
			numMems++;
		} else if (VarNode* VN = dyn_cast<VarNode>(Node)){
			Value* V = VN->getValue();

			if (isa<ConstantInt>(V)){
				numConstants++;
			} else {

				//Only Variables are considered for bitwidth reduction
				isIntegerVariable = V->getType()->isIntegerTy();
				numVars++;
				if(isIntegerVariable){
					currentBitWidth = V->getType()->getPrimitiveSizeInBits();
					usedBits += currentBitWidth;
				}
			}

		} else assert(false && "Unknown Node Type");

		assert(out_state.count(Node) && "Node not found in the list of computed ranges.");

		Range CR = out_state[Node];

		// If range is unknown, we have total needed bits
		if (CR.isUnknown()) {
			++numUnknown;
			if (isIntegerVariable) needBits += currentBitWidth;
			continue;
		}

		// If range is empty, we have 0 needed bits
		if (CR.isEmpty()) {
			++numEmpty;
			continue;
		}

		if (CR.getLower().eq(Min)) {
			if (CR.getUpper().eq(Max)) {
				++numMaxRange;
			}
			else {
				++numMinInfC;
			}
		}
		else if (CR.getUpper().eq(Max)) {
			++numCPlusInf;
		}
		else {
			++numCC;
		}

		//Compute needed bits >> only for variables
		if (isIntegerVariable) {
			unsigned ub, lb;

			if (CR.getLower().isNegative()) {
				APInt abs = CR.getLower().abs();
				lb = abs.getActiveBits() + 1;
			} else {
				lb = CR.getLower().getActiveBits() + 1;
			}

			if (CR.getUpper().isNegative()) {
				APInt abs = CR.getUpper().abs();
				ub = abs.getActiveBits() + 1;
			} else {
				ub = CR.getUpper().getActiveBits() + 1;
			}

			unsigned nBits = lb > ub ? lb : ub;

			// If both bounds are positive, decrement needed bits by 1
			if (!CR.getLower().isNegative() && !CR.getUpper().isNegative()) {
				--nBits;
			}

			if (nBits < currentBitWidth) {
				needBits += nBits;
			} else {
				needBits += currentBitWidth;
			}
		}


	}

	double totalB = usedBits;
	double needB = needBits;
	double reduction = (double) (totalB - needB) * 100 / totalB;
	percentReduction = (unsigned int) reduction;

}

/*
 * function getInitialState
 *
 * Returns the initial state of a node in the Graph.
 */
Range llvm::RangeAnalysis::getInitialState(GraphNode* Node){
	if (initial_state.count(Node)) return initial_state[Node];
	return Range(Min, Max, Unknown);
}

/*
 * method printSCCState
 *
 * Prints the abstract state of all the nodes of a SCC
 */
void llvm::RangeAnalysis::printSCCState(int SCCid){

	SCC_Iterator it(depGraph, SCCid);

	while(it.hasNext()){

		GraphNode* node = it.getNext();

		errs() << node->getLabel() << "	";
		out_state[node].print(errs());
		errs() << "\n";

	}
}


/*
 * method fixPointIteration
 *
 * Implements a worklist algorithm that updates the abstract states
 * of the nodes of the graph until a fixed point is reached.
 *
 * The LatticeOperation lo determines if the operation will be a widening
 * or narrowing analysis.
 */
void llvm::RangeAnalysis::fixPointIteration(int SCCid, LatticeOperation lo) {

	SCC_Iterator it(depGraph, SCCid);

	std::set<GraphNode*> worklist;

	std::list<GraphNode*> currentSCC;

	while(it.hasNext()){

		GraphNode* node = it.getNext();

		//Things to do in the first fixPoint iteration (growth analysis)
		if (lo == loJoin) {
			currentSCC.push_back(node);

			//Initialize abstract state
			out_state[node] = getInitialState(node);
			widening_count[node] = 0;
			narrowing_count[node] = 0;
		}

		std::map<GraphNode*, edgeType> preds = node->getPredecessors();
		if (preds.size() == 0) {
			worklist.insert(node);
		} else {

			//Add the Sigma Nodes to the worklist of the narrowing,
			//even if they do not receive data from outside the SCC
			if (lo == loMeet) {
				if(SigmaOpNode* Sigma = dyn_cast<SigmaOpNode>(node)){
					worklist.insert(Sigma);
				}
			} else {

				//Look for nodes that receive information from outside the SCC
				for(std::map<GraphNode*, edgeType>::iterator pred = preds.begin(), pred_end = preds.end(); pred != pred_end; pred++){
					//Only data dependence edges
					if(pred->second != etData) continue;

					if(depGraph->getSCCID(pred->first) != SCCid) {
						worklist.insert(node);
						break;
					}
				}
			}
		}
	}

	while(worklist.size() > 0){

		GraphNode* currentNode = *(worklist.begin());
		worklist.erase(currentNode);

		computeNode(currentNode, worklist, lo);

	}

	if (lo == loJoin){

		//Statistic: size of SCCs
		unsigned int current_size = currentSCC.size();
		if (current_size == 1) numAloneSCCs++;
		else if (current_size > sizeMaxSCC) sizeMaxSCC = current_size;

		for(std::list<GraphNode*>::iterator it = currentSCC.begin(), iend = currentSCC.end(); it != iend; it++){
			GraphNode* currentNode = *it;

			if(out_state[currentNode].isUnknown()){
				out_state[currentNode] = Range(Min, Max);
			}

		}
	}

}

Range llvm::RangeAnalysis::getRange(Value* V) {

	if (GraphNode* Node = depGraph->findNode(V) )
		return out_state[Node];

	//Value not found. Return [-inf,+inf]
	//errs() << "Requesting range for unknown value: " << V << "\n";
	return Range();
}

/*************************************************************************
 * Class IntraProceduralRA
 *************************************************************************/

void IntraProceduralRA::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<functionDepGraph> ();
    AU.addRequired<BranchAnalysis> ();
    if(!RAFilename.empty()) AU.addRequired<ModuleLookup> ();
    AU.setPreservesAll();
}

bool IntraProceduralRA::runOnFunction(Function& F) {

	//Build intra-procedural dependence graph
	functionDepGraph& M_DepGraph = getAnalysis<functionDepGraph>();
	depGraph = M_DepGraph.depGraph;

	if(!RAFilename.empty()) {
		ModuleLookup& ML = getAnalysis<ModuleLookup>();
		importInitialStates(ML);
	}

	//Extract constraints from branch conditions
	BranchAnalysis& brAnalysis = getAnalysis<BranchAnalysis>();
	std::map<const Value*, std::list<ValueSwitchMap*> > constraints = brAnalysis.getIntervalConstraints();

	//Add branch information to the dependence graph. Here we add the future values
	addConstraints(constraints);

	//Solve range analysis
	solve();

	return false;
}

char IntraProceduralRA::ID = 0;
static RegisterPass<IntraProceduralRA> X("ra-intra",
		"Intra-procedural Range Analysis");

/*************************************************************************
 * Class InterProceduralRA
 *************************************************************************/

void InterProceduralRA::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<moduleDepGraph> ();
    AU.addRequired<BranchAnalysis> ();
    if(!RAFilename.empty()) AU.addRequired<ModuleLookup> ();
    AU.setPreservesAll();
}

bool InterProceduralRA::runOnModule(Module& M) {

	//Build inter-procedural dependence graph
	moduleDepGraph& M_DepGraph = getAnalysis<moduleDepGraph>();
	depGraph = M_DepGraph.depGraph;

	if(!RAFilename.empty()) {
		ModuleLookup& ML = getAnalysis<ModuleLookup>();
		importInitialStates(ML);
	}

	loadIgnoredFunctions(RAIgnoredFunctions);
	addIgnoredFunction("malloc");

	for(Module::iterator Fit = M.begin(), Fend = M.end(); Fit != Fend; Fit++){

		//Skip functions without body (externally linked functions, such as printf)
		if (Fit->begin() == Fit->end()) continue;

		//Extract constraints from branch conditions
		BranchAnalysis& brAnalysis = getAnalysis<BranchAnalysis>(*Fit);
		std::map<const Value*, std::list<ValueSwitchMap*> > constraints = brAnalysis.getIntervalConstraints();

		//Add branch information to the dependence graph. Here we add the future values
		addConstraints(constraints);

	}

	//Solve range analysis
	solve();


//	errs() << "Computed ranges:\n";
//
//	for(Module::iterator Fit = M.begin(), Fend = M.end(); Fit != Fend; Fit++){
//
//		for (Function::iterator BBit = Fit->begin(), BBend = Fit->end(); BBit != BBend; BBit++){
//
//			for (BasicBlock::iterator Iit = BBit->begin(), Iend = BBit->end(); Iit != Iend; Iit++){
//
//				Instruction* I = Iit;
//
//				Range R = getRange(I);
//
//				errs() << I->getName() << "	";
//				R.print(errs());
//				errs() << "\n";
//			}
//
//		}
//	}


	return false;
}

char InterProceduralRA::ID = 0;
static RegisterPass<InterProceduralRA> Y("ra-inter",
		"Inter-procedural Range Analysis");
