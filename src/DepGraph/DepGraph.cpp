#include "DepGraph.h"


using namespace llvm;

static cl::opt<bool, false>
includeAllInstsInDepGraph("includeAllInstsInDepGraph", cl::desc("Include All Instructions In DepGraph."), cl::NotHidden);



//*********************************************************************************************************************************************************************
//                                                                                                                              DEPENDENCE GRAPH API
//*********************************************************************************************************************************************************************
//
// Author: Raphael E. Rodrigues
// Contact: raphaelernani@gmail.com
// Date: 11/03/2013
// Last update: 11/03/2013
// Project: e-CoSoc
// Institution: Computer Science department of Federal University of Minas Gerais
//
//*********************************************************************************************************************************************************************

//FIXME: Deal properly with invoke instructions. An Invoke instruction can be treated as a call node

/*
 * Class GraphNode
 */

GraphNode::GraphNode() {
        Class_ID = GraphNodeId;
        ID = currentID++;
}

GraphNode::GraphNode(GraphNode &G) {
        Class_ID = GraphNodeId;
        ID = currentID++;
}

GraphNode::~GraphNode() {

        for (std::map<GraphNode*, edgeType>::iterator pred = predecessors.begin(); pred
                        != predecessors.end(); pred++) {
                (*pred).first->successors.erase(this);
                NrEdges--;
        }

        for (std::map<GraphNode*, edgeType>::iterator succ = successors.begin(); succ
                        != successors.end(); succ++) {
                (*succ).first->predecessors.erase(this);
                NrEdges--;
        }

        successors.clear();
        predecessors.clear();
}

std::map<GraphNode*, edgeType> llvm::GraphNode::getSuccessors() {
        return successors;
}

std::map<GraphNode*, edgeType> llvm::GraphNode::getPredecessors() {
        return predecessors;
}

void llvm::GraphNode::connect(GraphNode* dst, edgeType type) {

        unsigned int curSize = this->successors.size();
        this->successors[dst] = type;
        dst->predecessors[this] = type;

        if (this->successors.size() != curSize) //Only count new edges
                NrEdges++;
}

void llvm::GraphNode::disconnect(GraphNode* dst) {

    unsigned int curSize = this->successors.size();

    if (this->successors.find(dst) != this->successors.end()) {
    	this->successors.erase(dst);
    	dst->predecessors.erase(this);
    }

    if (this->successors.size() != curSize) //Preventing errors
            NrEdges--;

}


NodeClassId llvm::GraphNode::getClass_Id() const {
        return Class_ID;
}

int llvm::GraphNode::getId() const {
        return ID;
}

bool llvm::GraphNode::hasSuccessor(GraphNode* succ) {
        return successors.count(succ) > 0;
}

bool llvm::GraphNode::hasPredecessor(GraphNode* pred) {
        return predecessors.count(pred) > 0;
}

std::string llvm::GraphNode::getName() {
        std::ostringstream stringStream;
        stringStream << "node_" << getId();
        return stringStream.str();
}

std::string llvm::GraphNode::getStyle() {
        return std::string("solid");
}

llvm::raw_ostream& GraphNode::dump(llvm::raw_ostream &strm){
	return strm << "GraphNode(	ID=" 			<< this->getId() <<
			                   "\n		ClassID=" 		<< this->getClass_Id() <<
			                   "\n		Label=" 		<< this->getLabel() <<
			                   "\n		Successors=" 	<< this->successors.size() <<
			                   "\n		Predecessors=" 	<< this->predecessors.size() << ")";
}

//llvm::raw_ostream& operator<<(llvm::raw_ostream &strm, GraphNode &a) {
//  return a.dump(strm);
//}

std::string llvm::GraphNode::getClassName(NodeClassId classID) {
	switch(classID){
	case GraphNodeId: return "GraphNode";
	case VarNodeId: return "VarNode";
	case OpNodeId: return "OpNode";
	case CallNodeId: return "CallNode";
	case MemNodeId: return "MemNode";
	case BinaryOpNodeId: return "BinaryOpNode";
	case UnaryOpNodeId: return "UnaryOpNode";
	case SigmaOpNodeId: return "SigmaOpNode";
	case PHIOpNodeId: return "PHIOpNode";
	}
	return "Unknown Class";
}

int llvm::GraphNode::currentID = 0;

/*
 * Class OpNode
 */
unsigned int OpNode::getOpCode() const {
    return OpCode;
}

void OpNode::setOpCode(unsigned int opCode) {
    if (!inst) OpCode = opCode;
}

std::string llvm::OpNode::getLabel() {

        std::ostringstream stringStream;
        stringStream << getId() << " " << Instruction::getOpcodeName(OpCode);
        return stringStream.str();

}

std::string llvm::OpNode::getShape() {
        return std::string("octagon");
}

GraphNode* llvm::OpNode::clone() {

	OpNode* R = new OpNode(*this);
	R->Class_ID = this->Class_ID;
	return R;

}

GraphNode* llvm::OpNode::getIncomingNode(unsigned int index, edgeType et){

	GraphNode* result = NULL;

	std::map<GraphNode*, edgeType>::iterator pred, pred_end;
	for(pred = predecessors.begin(), pred_end = predecessors.end(); pred != pred_end && index >= 0; pred++){

		if(pred->second == et){

			if(index == 0) {
				result = pred->first;
			}

			index--;

		}

	}

	return result;

}

GraphNode* llvm::OpNode::getOperand(unsigned int index){

	GraphNode* result = NULL;

	if (Value* V = inst->getOperand(index)){

		std::map<GraphNode*, edgeType>::iterator pred, pred_end;
		for(pred = predecessors.begin(), pred_end = predecessors.end(); pred != pred_end; pred++){

			if (VarNode* VN = dyn_cast<VarNode>(pred->first)) {

				if(V == VN->getValue()) {
					result = pred->first;
					break;
				}
			}
		}
	}

	return result;

}

/*
 * Class PHIOpNode
 */
std::string llvm::PHIOpNode::getLabel() {
        std::ostringstream stringStream;

        stringStream << "PHI ";
        if (PHI->hasName())
                stringStream << "(" << PHI->getName().str() << ")";
        else
                stringStream << "(Unnamed)";

        return stringStream.str();
}

std::string llvm::PHIOpNode::getShape() {
        return std::string("octagon");
}

GraphNode* llvm::PHIOpNode::clone() {
	PHIOpNode* R = new PHIOpNode(*this);
	R->Class_ID = this->Class_ID;
	return R;
}

std::string llvm::PHIOpNode::getStyle() {
        return std::string("dashed");
}

/*
 * Class BinaryOpNode
 */
GraphNode* llvm::BinaryOpNode::clone() {
	BinaryOpNode* R = new BinaryOpNode(*this);
	R->Class_ID = this->Class_ID;
	return R;
}

/*
 * Class UnaryOpNode
 */
GraphNode* llvm::UnaryOpNode::clone() {
	UnaryOpNode* R = new UnaryOpNode(*this);
	R->Class_ID = this->Class_ID;
	return R;
}

/*
 * Class PHIOpNode
 */
std::string llvm::SigmaOpNode::getLabel() {
        std::ostringstream stringStream;

        stringStream << "Sigma ";
        if (Sigma->hasName())
                stringStream << "(" << Sigma->getName().str() << ")";
        else
                stringStream << "(Unnamed)";

        return stringStream.str();
}

std::string llvm::SigmaOpNode::getShape() {
        return std::string("octagon");
}

GraphNode* llvm::SigmaOpNode::clone() {
	SigmaOpNode* R = new SigmaOpNode(*this);
	R->Class_ID = this->Class_ID;
	return R;
}

std::string llvm::SigmaOpNode::getStyle() {
        return std::string("dotted");
}


/*
 * Class CallNode
 */
Function* llvm::CallNode::getCalledFunction() const {
        return CI->getCalledFunction();
}

std::string llvm::CallNode::getLabel() {
        std::ostringstream stringStream;

        stringStream << "Call ";
        if (Function* F = getCalledFunction())
                stringStream << F->getName().str();
        else if (CI->hasName())
                stringStream << "*(" << CI->getName().str() << ")";
        else
                stringStream << "*(Unnamed)";

        return stringStream.str();
}

std::string llvm::CallNode::getShape() {
        return std::string("doubleoctagon");
}

GraphNode* llvm::CallNode::clone() {
	CallNode* R = new CallNode(*this);
	R->Class_ID = this->Class_ID;
	return R;
}

CallInst* llvm::CallNode::getCallInst() const {
	return this->CI;
}

/*
 * Class VarNode
 */
llvm::Value* VarNode::getValue() {
        return value;
}

std::string llvm::VarNode::getShape() {

        if (!isa<Constant> (value)) {
                return std::string("ellipse");
        } else {
                return std::string("box");
        }

}

std::string llvm::VarNode::getLabel() {

        std::ostringstream stringStream;

        if (!isa<Constant> (value)) {

                stringStream << value->getName().str();

        } else {

                if ( ConstantInt* CI = dyn_cast<ConstantInt>(value)) {
                        stringStream << CI->getValue().toString(10, true);
                } else {
                        stringStream << "Const:" << value->getName().str();
                }
        }

        return stringStream.str();

}

GraphNode* llvm::VarNode::clone() {
	VarNode* R = new VarNode(*this);
    	R->Class_ID = this->Class_ID;
    	return R;
}

/*
 * Class MemNode
 */
std::set<llvm::Value*> llvm::MemNode::getAliases() {
	std::set<llvm::Value*> aliases;
	return USE_ALIAS_SETS ? AS->getValueSets()[aliasSetID] : aliases;
}

std::string llvm::MemNode::getLabel() {
        std::ostringstream stringStream;
        stringStream << "Memory " << aliasSetID;
        return stringStream.str();
}

std::string llvm::MemNode::getShape() {
        return std::string("ellipse");
}

GraphNode* llvm::MemNode::clone() {
	MemNode* R = new MemNode(*this);
    	R->Class_ID = this->Class_ID;
    	return R;
}

std::string llvm::MemNode::getStyle() {
        return std::string("dashed");
}

int llvm::MemNode::getAliasSetId() const {
        return aliasSetID;
}


/*
 * Class DepGraph
 */
std::set<GraphNode*>::iterator DepGraph::begin(){
	return(nodes.begin());
}

std::set<GraphNode*>::iterator DepGraph::end(){
	return(nodes.end());
}

DepGraph::~DepGraph() {
        nodes.clear();
}

DepGraph* DepGraph::getParentGraph(){
	return parentGraph;
}

DepGraph DepGraph::generateSubGraph(Value *src, Value *dst) {

        GraphNode* source = findOpNode(src);
        if (!source) source = findNode(src);

        GraphNode* destination = findNode(dst);

        return generateSubGraph(source, destination);
}


DepGraph DepGraph::generateSubGraph(GraphNode* source, GraphNode* destination) {

	std::set<GraphNode*> intersection;

	if (nodes.count(source) && nodes.count(destination)){;

		std::set<GraphNode*> visitedNodes1;
		std::set<GraphNode*> visitedNodes2;

		dfsVisit(source, visitedNodes1);
		dfsVisitBack(destination, visitedNodes2);

		//check the nodes visited in both directions
		for (std::set<GraphNode*>::iterator it = visitedNodes1.begin(); it != visitedNodes1.end(); ++it) {
			if (visitedNodes2.count(*it) > 0) {
				intersection.insert(*it);
			}
		}
	}

	return makeSubGraph(intersection);
}

/*
 * SubGraph containing only the SCC
 */
DepGraph DepGraph::generateSubGraph(int SCCID){

	return makeSubGraph(sCCs[SCCID]);

}

//Creates an entirely new graph, with equivalent nodes and edges
DepGraph* DepGraph::clone(){


	DepGraph* result;

	result = new DepGraph(AS); //Somebody has to free this memory at some point in the future
	*result = makeSubGraph(nodes);
	result->parentGraph = NULL;  //Make the graphs independent

	result->sCCs.clear();
	result->reverseSCCMap.clear();

	return result;

}

DepGraph DepGraph::makeSubGraph(std::set<GraphNode*> nodeList){

    DepGraph G(this->AS);
    G.parentGraph = this;

    if (!nodeList.size()) return G;

    //Create map of new and original nodes
    for (std::set<GraphNode*>::iterator it = nodeList.begin(); it != nodeList.end(); ++it) {
    	G.nodeMap[*it] = (*it)->clone();
    }

    //Copy the vertices
    for (std::map<GraphNode*, GraphNode*>::iterator it = G.nodeMap.begin(); it != G.nodeMap.end(); ++it) {

            std::map<GraphNode*, edgeType> succs = it->first->getSuccessors();

            for (std::map<GraphNode*, edgeType>::iterator succ = succs.begin(), s_end = succs.end(); succ != s_end; succ++) {
                    if (G.nodeMap.count(succ->first)) {
                            it->second->connect(G.nodeMap[succ->first], succ->second);
                    }
            }


            //Copy the nodes into the new graph
            if ( !G.nodes.count(it->second)) {
            	G.nodes.insert(it->second);

            	if (isa<VarNode>(it->second)) {
            		G.varNodes[dyn_cast<VarNode>(it->second)->getValue()] = dyn_cast<VarNode>(it->second);
            	}

            	if (isa<MemNode>(it->second)) {
            		G.memNodes[dyn_cast<MemNode>(it->second)->getAliasSetId()] = dyn_cast<MemNode>(it->second);
            	}

            	if (isa<OpNode>(it->second)) {
            		G.opNodes[dyn_cast<OpNode>(it->second)->getOperation()] = dyn_cast<OpNode>(it->second);

            		if (isa<CallNode>(it->second)) {
                		G.callNodes[dyn_cast<CallNode>(it->second)->getCallInst()] = dyn_cast<CallNode>(it->second);

                	}
            	}

            }
    }

    G.recomputeSCCs();

    return G;

}

void DepGraph::dfsVisit(GraphNode* u, std::set<GraphNode*> &visitedNodes) {

        visitedNodes.insert(u);

        std::map<GraphNode*, edgeType> succs = u->getSuccessors();

        for (std::map<GraphNode*, edgeType>::iterator succ = succs.begin(), s_end =
                        succs.end(); succ != s_end; succ++) {
                if (visitedNodes.count(succ->first) == 0) {
                        dfsVisit(succ->first, visitedNodes);
                }
        }

}



void DepGraph::dfsVisitBack(GraphNode* u, std::set<GraphNode*> &visitedNodes) {

        visitedNodes.insert(u);

        std::map<GraphNode*, edgeType> preds = u->getPredecessors();

        for (std::map<GraphNode*, edgeType>::iterator pred = preds.begin(), s_end =
                        preds.end(); pred != s_end; pred++) {
                if (visitedNodes.count(pred->first) == 0) {
                        dfsVisitBack(pred->first, visitedNodes);
                }
        }

}






void DepGraph::dfsVisitBack_ext(GraphNode* u, std::set<GraphNode*> &visitedNodes, std::map<int, GraphNode*> &firstNodeVisitedPerSCC){

    visitedNodes.insert(u);
    int SCCID = getSCCID(u);

    if (!firstNodeVisitedPerSCC.count(SCCID)) firstNodeVisitedPerSCC[SCCID] = u;

    std::map<GraphNode*, edgeType> preds = u->getPredecessors();

    for (std::map<GraphNode*, edgeType>::iterator pred = preds.begin(), s_end =
                    preds.end(); pred != s_end; pred++) {
		if (visitedNodes.count(pred->first) == 0) {
			dfsVisitBack_ext(pred->first, visitedNodes, firstNodeVisitedPerSCC);
		}
    }

}

//Here we look for a back edge that leads to a node different than the first node.
bool lookForNestedLoop( GraphNode* first,
						GraphNode* current,
						std::set<GraphNode*> &currentpath ,
						std::set<GraphNode*> &visitedNodes) {

    bool found = false;

	visitedNodes.insert(current);

	currentpath.insert(current);

	std::map<GraphNode*, edgeType> succs = current->getSuccessors();

	for (std::map<GraphNode*, edgeType>::iterator succ = succs.begin(), s_end =
					succs.end(); succ != s_end; succ++) {

		if (succ->first != first && currentpath.count(succ->first)) {
			found = true;
			break;
		}
		if (!visitedNodes.count(succ->first)) {
			if (lookForNestedLoop(first, succ->first, currentpath, visitedNodes) ){
				found = true;
				break;
			}
		}
	}

	currentpath.erase(current);

    return found;

}


bool DepGraph::hasNestedLoop(GraphNode* first){
	std::set<GraphNode*> currentpath;
	std::set<GraphNode*> visitedNodes;

	return lookForNestedLoop( first, first, currentpath, visitedNodes);
}

bool DepGraph::hasNestedLoop(int SCCID){
	std::set<GraphNode*> SCC = getSCC(SCCID);
	GraphNode* first = *(SCC.begin());
	return hasNestedLoop(first);
}



//Print the graph (.dot format) in the stderr stream.
void DepGraph::toDot(std::string s) {

        this->toDot(s, &errs());

}

void DepGraph::toDot(std::string s, const std::string fileName) {

        std::string ErrorInfo;

        raw_fd_ostream File(fileName.c_str(), ErrorInfo);

        if (!ErrorInfo.empty()) {
                errs() << "Error opening file " << fileName
                                << " for writing! Error Info: " << ErrorInfo << " \n";
                return;
        }

        this->toDot(s, &File);

}

void DepGraph::toDot(std::string s, raw_ostream *stream) {

        (*stream) << "digraph \"DFG for \'" << s << "\' function \"{\n";
        (*stream) << "label=\"DFG for \'" << s << "\' function\";\n";

        std::map<GraphNode*, int> DefinedNodes;

        for (std::set<GraphNode*>::iterator node = nodes.begin(), end = nodes.end(); node
                        != end; node++) {

                if (DefinedNodes.count(*node) == 0) {
                        (*stream) << (*node)->getName() << "[shape=" << (*node)->getShape()
                                        << ",style=" << (*node)->getStyle() << ",label=\""
                                        << (*node)->getLabel() << "\"]\n";
                        DefinedNodes[*node] = 1;
                }

                std::map<GraphNode*, edgeType> succs = (*node)->getSuccessors();

                for (std::map<GraphNode*, edgeType>::iterator succ = succs.begin(),
                                s_end = succs.end(); succ != s_end; succ++) {

                        if (DefinedNodes.count(succ->first) == 0) {
                                (*stream) << (succ->first)->getName() << "[shape="
                                                << (succ->first)->getShape() << ",style="
                                                << (succ->first)->getStyle() << ",label=\""
                                                << (succ->first)->getLabel() << "\"]\n";
                                DefinedNodes[succ->first] = 1;
                        }

                        //Source
                        (*stream) << "\"" << (*node)->getName() << "\"";

                        (*stream) << "->";

                        //Destination
                        (*stream) << "\"" << (succ->first)->getName() << "\"";

                        if (succ->second == etControl)
                                (*stream) << " [style=dashed]";

                        (*stream) << "\n";

                }

        }

        (*stream) << "}\n\n";

}

void llvm::DepGraph::toDot(std::string s, raw_ostream *stream, llvm::DepGraph::Guider* g) {
        (*stream) << "digraph \"DFG for \'" << s << "\' module \"{\n";
        (*stream) << "label=\"DFG for \'" << s << "\' module\";\n";

        // print every node
        for (std::set<GraphNode*>::iterator node = nodes.begin(), end = nodes.end(); node
                        != end; node++) {
                        (*stream) << (*node)->getName() << g->getNodeAttrs(*node) << "\n";

        }
        // print edges
        for (std::set<GraphNode*>::iterator node = nodes.begin(), end = nodes.end(); node
                                != end; node++) {
                std::map<GraphNode*, edgeType> succs = (*node)->getSuccessors();
                for (std::map<GraphNode*, edgeType>::iterator succ = succs.begin(),
                                s_end = succs.end(); succ != s_end; succ++) {
                        //Source
                        (*stream) << "\"" << (*node)->getName() << "\"";
                        (*stream) << "->";
                        //Destination
                        (*stream) << "\"" << (succ->first)->getName() << "\"";
                        (*stream) << g->getEdgeAttrs(*node, succ->first);
                        (*stream) << "\n";
                }
        }
        (*stream) << "}\n\n";
}

void DepGraph::removeNode(GraphNode* target){

	if (OpNode* on = dyn_cast<OpNode>(target)){
		opNodes.erase(on->getOperation());
		if (CallNode* cn = dyn_cast<CallNode>(target)){
			callNodes.erase(cn->getCallInst());
		}
	} else if (VarNode* vn = dyn_cast<VarNode>(target)){
		varNodes.erase(vn->getValue());
	} else if (MemNode* mn = dyn_cast<MemNode>(target)){
		memNodes.erase(mn->getAliasSetId());
	}

    nodes.erase(target);
    delete target;

}

const std::string sigmaString = "vSSA_sigma";

bool DepGraph::isSigma(const PHINode* Phi){
	if (Phi->getName().startswith(sigmaString)
		|| Phi->getMetadata(sigmaString) != 0
		|| Phi->getMetadata("Sigma") != 0) return true;

	return false;
}

GraphNode* DepGraph::addInst(Value *v) {

        GraphNode *Op, *Var, *Operand;

        Instruction* Inst = dyn_cast<Instruction>(v);
        CallInst* CI = dyn_cast<CallInst>(v);
        bool hasVarNode = true;

        if (isValidInst(v)) { //If is a data manipulator instruction
                Var = this->findNode(v);

                /*
                 * If Var is NULL, the value hasn't been processed yet, so we must process it
                 *
                 * However, if Var is a Pointer, maybe the memory node already exists but the
                 * operation node isn't in the graph, yet. Thus we must process it.
                 */
                if (Var == NULL || (Var != NULL && Inst != NULL && findOpNode(v) == NULL)) { //If it has not processed yet

                        //If Var isn't NULL, we won't create another node for it
                        if (Var == NULL) {

                                if (CI) {
                                        hasVarNode = !CI->getType()->isVoidTy();
                                }

                                if (hasVarNode) {
                                        if (StoreInst* SI = dyn_cast<StoreInst>(v))
                                                Var = addInst(SI->getOperand(1)); // We do this here because we want to represent the store instructions as a flow of information of a data to a memory node
                                        else if (isMemoryPointer(v)) {

                                        	Var = new MemNode(
                                                                USE_ALIAS_SETS ? AS->getValueSetKey(v) : 0, AS);
                                                memNodes[USE_ALIAS_SETS ? AS->getValueSetKey(v) : 0]
                                                                = Var;
                                        } else {
                                                Var = new VarNode(v);
                                                varNodes[v] = Var;
                                        }
                                        nodes.insert(Var);
                                }

                        }

                        if (Inst) {

                            //Here we create the OpNode, according to the type of the instruction

							if (CI) {
								Op = new CallNode(CI);
								callNodes[CI] = Op;
							} else if (BinaryOperator* BOP = dyn_cast<BinaryOperator>(Inst)) {
								Op = new BinaryOpNode(BOP);
                            } else if (UnaryInstruction* UOP = dyn_cast<UnaryInstruction>(Inst)) {
								Op = new UnaryOpNode(UOP);
                            } else if (PHINode* PHI = dyn_cast<PHINode>(Inst)) {
								if (isSigma(PHI)) Op = new SigmaOpNode(PHI);
								else Op = new PHIOpNode(PHI);
                            } else {
                                Op = new OpNode(Inst);
                            }
                            opNodes[Inst] = Op;






							nodes.insert(Op);
							if (hasVarNode)
									Op->connect(Var);

							//Connect the operands to the OpNode
							for (unsigned int i = 0; i < Inst->getNumOperands(); i++) {

									if (isa<StoreInst>(Inst) && i == 1)
											continue; // We do this here because we want to represent the store instructions as a flow of information of a data to a memory node

									Value *v1 = Inst->getOperand(i);
									Operand = this->addInst(v1);

									if (Operand != NULL) Operand->connect(Op);
							}
                        }
                }

                return Var;
        }
        return NULL;
}

void DepGraph::addEdge(GraphNode* src, GraphNode* dst, edgeType type) {

        nodes.insert(src);
        nodes.insert(dst);
        src->connect(dst, type);

}

//It verify if the instruction is valid for the dependence graph, i.e. just data manipulator instructions are important for dependence graph
bool DepGraph::isValidInst(const Value *v) {

	if ((!includeAllInstsInDepGraph) && isa<Instruction> (v)) {

		//List of instructions that we don't want in the graph
		switch (cast<Instruction>(v)->getOpcode()) {

			case Instruction::Br:
			case Instruction::Switch:
			case Instruction::Ret:
				return false;

		}

	}

	if (v) return true;
	return false;

}

bool llvm::DepGraph::isMemoryPointer(const llvm::Value* v) {
        if (v && v->getType())
                return v->getType()->isPointerTy();
        return false;
}

//Return the pointer to the node related to the operand.
//Return NULL if the operand is not inside map.
GraphNode* DepGraph::findNode(const Value *op) {

        if (isMemoryPointer(op)) {
                int index = USE_ALIAS_SETS ? AS->getValueSetKey(op) : 0;
                if (memNodes.count(index))
                        return memNodes[index];
        } else {
                if (varNodes.count(op))
                        return varNodes[op];
        }

        return NULL;
}

GraphNode* DepGraph::findNode(GraphNode* node) {

		if (nodes.count(node)) return node;
		if (nodeMap.count(node)) return nodeMap[node];

        return NULL;
}

std::set<GraphNode*> DepGraph::findNodes(std::set<Value*> values) {

        std::set<GraphNode*> result;

        for (std::set<Value*>::iterator i = values.begin(), end = values.end(); i
                        != end; i++) {

                if (GraphNode* node = findNode(*i)) {
                        result.insert(node);
                }

        }

        return result;
}

OpNode* llvm::DepGraph::findOpNode(const llvm::Value* op) {

        if (opNodes.count(op))
                return dyn_cast<OpNode> (opNodes[op]);
        return NULL;
}

void llvm::DepGraph::deleteCallNodes(Function* F) {

        for (Value::use_iterator UI = F->use_begin(), E = F->use_end(); UI != E; ++UI) {
                User *U = *UI;

                // Ignore blockaddress uses
                if (isa<BlockAddress> (U))
                        continue;

                // Used by a non-instruction, or not the callee of a function, do not
                // match.

                //FIXME: Deal properly with invoke instructions
                if (!isa<CallInst> (U))
                        continue;

                Instruction *caller = cast<Instruction> (U);

                if (callNodes.count(caller)) {
                        if (GraphNode* node = callNodes[caller]) {
                                nodes.erase(node);
                                delete node;
                        }
                        callNodes.erase(caller);
                }

        }

}

std::pair<GraphNode*, int> llvm::DepGraph::getNearestDependency(llvm::Value* sink,
                std::set<llvm::Value*> sources, bool skipMemoryNodes) {

        std::pair<llvm::GraphNode*, int> result;
        result.first = NULL;
        result.second = -1;

        if (GraphNode* startNode = findNode(sink)) {

                std::set<GraphNode*> sourceNodes = findNodes(sources);

                std::map<GraphNode*, int> nodeColor;

                std::list<std::pair<GraphNode*, int> > workList;

                for (std::set<GraphNode*>::iterator Nit = nodes.begin(), Nend =
                                nodes.end(); Nit != Nend; Nit++) {

                        if (skipMemoryNodes && isa<MemNode> (*Nit))
                                nodeColor[*Nit] = 1;
                        else
                                nodeColor[*Nit] = 0;
                }

                workList.push_back(pair<GraphNode*, int> (startNode, 0));

                /*
                 * we will do a breadth search on the predecessors of each node,
                 * until we find one of the sources. If we don't find any, then the
                 * sink doesn't depend on any source.
                 */

                while (workList.size()) {

                        GraphNode* workNode = workList.front().first;
                        int currentDistance = workList.front().second;

                        nodeColor[workNode] = 1;

                        workList.pop_front();

                        if (sourceNodes.count(workNode)) {

                                result.first = workNode;
                                result.second = currentDistance;
                                break;

                        }

                        std::map<GraphNode*, edgeType> preds = workNode->getPredecessors();

                        for (std::map<GraphNode*, edgeType>::iterator pred = preds.begin(),
                                        pend = preds.end(); pred != pend; pred++) {

                                if (nodeColor[pred->first] == 0) { // the node hasn't been processed yet

                                        nodeColor[pred->first] = 1;

                                        workList.push_back(
                                                        pair<GraphNode*, int> (pred->first,
                                                                        currentDistance + 1));

                                }

                        }

                }

        }

        return result;
}

std::map<GraphNode*, std::vector<GraphNode*> > llvm::DepGraph::getEveryDependency(
                llvm::Value* sink, std::set<llvm::Value*> sources, bool skipMemoryNodes) {

        std::map<llvm::GraphNode*, std::vector<GraphNode*> > result;
        DenseMap<GraphNode*, GraphNode*> parent;
        std::vector<GraphNode*> path;

        //      errs() << "--- Get every dep --- \n";
        if (GraphNode* startNode = findNode(sink)) {
                //              errs() << "found sink\n";
                std::set<GraphNode*> sourceNodes = findNodes(sources);
                std::map<GraphNode*, int> nodeColor;
                std::list<GraphNode*> workList;
                //              int size = 0;
                for (std::set<GraphNode*>::iterator Nit = nodes.begin(), Nend =
                                nodes.end(); Nit != Nend; Nit++) {
                        //                      size++;
                        if (skipMemoryNodes && isa<MemNode> (*Nit))
                                nodeColor[*Nit] = 1;
                        else
                                nodeColor[*Nit] = 0;
                }

                workList.push_back(startNode);
                nodeColor[startNode] = 1;
                /*
                 * we will do a breadth search on the predecessors of each node,
                 * until we find one of the sources. If we don't find any, then the
                 * sink doesn't depend on any source.
                 */
                //              int pb = 1;
                while (!workList.empty()) {
                        GraphNode* workNode = workList.front();
                        workList.pop_front();
                        if (sourceNodes.count(workNode)) {
                                //Retrieve path
                                path.clear();
                                GraphNode* n = workNode;
                                path.push_back(n);
                                while (parent.count(n)) {
                                        path.push_back(parent[n]);
                                        n = parent[n];
                                }
                                std::reverse(path.begin(), path.end());
                                //                              errs() << "Path: ";
                                //                              for (std::vector<GraphNode*>::iterator i = path.begin(), e = path.end(); i != e; ++i) {
                                //                                      errs() << (*i)->getLabel() << " | ";
                                //                              }
                                //                              errs() << "\n";
                                result[workNode] = path;
                        }
                        std::map<GraphNode*, edgeType> preds = workNode->getPredecessors();
                        for (std::map<GraphNode*, edgeType>::iterator pred = preds.begin(),
                                        pend = preds.end(); pred != pend; pred++) {
                                if (nodeColor[pred->first] == 0) { // the node hasn't been processed yet
                                        nodeColor[pred->first] = 1;
                                        workList.push_back(pred->first);
                                        //                                      pb++;
                                        parent[pred->first] = workNode;
                                }
                        }
                        //                      errs() << pb << "/" << size << "\n";
                }
        }
        return result;
}


int llvm::DepGraph::getNumOpNodes() {
	return opNodes.size();
}

int llvm::DepGraph::getNumCallNodes() {
	return callNodes.size();
}

int llvm::DepGraph::getNumMemNodes() {
	return memNodes.size();
}

int llvm::DepGraph::getNumVarNodes() {
	return varNodes.size();
}

int llvm::DepGraph::getNumEdges(edgeType type){

	int result = 0;

    for (std::set<GraphNode*>::iterator node = nodes.begin(), end = nodes.end(); node
                     != end; node++) {

             std::map<GraphNode*, edgeType> succs = (*node)->getSuccessors();

             for (std::map<GraphNode*, edgeType>::iterator succ = succs.begin(),
                             s_end = succs.end(); succ != s_end; succ++) {

                     if (succ->second == type) result++;

             }

     }

    return result;

}

int llvm::DepGraph::getNumDataEdges() {
	return getNumEdges(etData);
}

int llvm::DepGraph::getNumControlEdges() {
	return getNumEdges(etControl);
}

std::list<GraphNode*> llvm::DepGraph::getNodesWithoutPredecessors() {

	std::list<GraphNode*> result;

    for (std::set<GraphNode*>::iterator node = nodes.begin(), end = nodes.end(); node != end; node++) {

             std::map<GraphNode*, edgeType> preds = (*node)->getPredecessors();

             if (preds.size() == 0) result.push_back(*node);

     }

	return result;

}

void llvm::DepGraph::removeEdge(GraphNode* src, GraphNode* dst) {

	src->disconnect(dst);

}



void llvm::DepGraph::strongconnect(GraphNode* node,
		           std::map<GraphNode*, int> &index,
		           std::map<GraphNode*, int> &lowlink,
		           int &currentIndex,
		           std::stack<GraphNode*> &S,
		           std::set<GraphNode*> &S2,
		           std::map<int, std::set<GraphNode*> > &SCCs){


    // Set the depth index for node to the smallest unused index
	index[node] = currentIndex;
    lowlink[node] = currentIndex;
    currentIndex++;

    S.push(node);
    S2.insert(node);

    // Consider successors of v
	std::map<GraphNode*, edgeType> succs = node->getSuccessors();
	for (std::map<GraphNode*, edgeType>::iterator succ = succs.begin(), s_end =
					succs.end(); succ != s_end; succ++) {

	       if (!index.count(succ->first)){
	         // Successor succ has not yet been visited; recurse on it
	        strongconnect(succ->first, index, lowlink, currentIndex, S, S2, SCCs);
	        lowlink[node]  = min(lowlink[node], lowlink[succ->first]);
	      } else if (S2.count(succ->first)) {
	         // Successor w is in stack S and hence in the current SCC
	         lowlink[node] = min(lowlink[node], index[succ->first]);
	      }
	}


    // If v is a root node, pop the stack and generate an SCC
    if (lowlink[node] == index[node]){
      //start a new strongly connected component
    	GraphNode* w;
    	do {
        	w = S.top();

        	S2.erase(w);
        	S.pop();

        	reverseSCCMap[w] = lowlink[node];
        	SCCs[lowlink[node]].insert(w);
    	} while (w != node);
      //output the current strongly connected component

    }

}

//SCC definition using the Tarjan's algorithm
void llvm::DepGraph::recomputeSCCs(){

	sCCs.clear();
	reverseSCCMap.clear();
	topologicalOrderedSCCs.clear();

	int currentIndex = 0;
	std::map<GraphNode*, int> index;
	std::map<GraphNode*, int> lowlink;

	std::stack<GraphNode*> S;
	std::set<GraphNode*> S2;

	for (std::set<GraphNode*>::iterator node = nodes.begin(), end = nodes.end(); node != end; node++) {

		if(!index.count(*node)){
			strongconnect(*node, index, lowlink, currentIndex, S, S2, sCCs);
		}

	}

}

void llvm::DepGraph::printSCC(int SCCid, raw_ostream& OS){

	OS << "SCC " << SCCid << " {\n";

	for(std::set<GraphNode*>::iterator it = sCCs[SCCid].begin(); it != sCCs[SCCid].end(); it++){
		GraphNode* Node = *it;
		OS <<  "	" <<  Node->getLabel() << ": "
				      << GraphNode::getClassName(Node->getClass_Id())
		              << " [ ID = "<< Node->getId() << " ]\n";
	}
	OS << "} \n";

}


void llvm::DepGraph::dumpSCCs(){

	errs() << "\nSCCs\n";
	for(std::map<int, std::set<GraphNode*> >::iterator it = sCCs.begin(); it != sCCs.end(); it++){
		errs() << "SCC[" << it->first << "] : " << it->second.size() << " nodes\n"  ;
	}

	errs() << "\nNodes\n";
	for(std::map<GraphNode*, int>::iterator it = reverseSCCMap.begin(); it != reverseSCCMap.end(); it++){
		errs() << "Node[" << it->first->getLabel() << "] : SCC " << it->second << "\n"  ;
	}

}

std::map<int, std::set<GraphNode*> > llvm::DepGraph::getSCCs(){

	if(!sCCs.size()) {

		recomputeSCCs();
	}

	return sCCs;
}

std::list<int> llvm::DepGraph::getSCCTopologicalOrder(){

	std::map<int, std::set<int> > dagSCC;


	if (!topologicalOrderedSCCs.size()) {

		recomputeSCCs();

		//Step 1: Build SCC DAG
		std::map<int, std::set<GraphNode*> >  localSCCs = getSCCs();

		//For each SCC....
		for (std::map<int, std::set<GraphNode*> >::iterator it = localSCCs.begin(); it != localSCCs.end(); it++){

			int currentSCC = it->first;

			//just to make sure it will be in the map
			dagSCC[currentSCC];

			//... iterate over its nodes...
			for ( std::set<GraphNode*>::iterator node = it->second.begin(); node != it->second.end(); node++ ){
				GraphNode* currentNode = *node;

				//... and create edges from the successor SCCs
				std::map<GraphNode*, edgeType> successors = currentNode->getSuccessors();
				for (std::map<GraphNode*, edgeType>::iterator succ = successors.begin(); succ != successors.end(); succ++){

					GraphNode* succNode = succ->first;
					int succSCC = getSCCID(succNode);

					// We are creating a DAG. Thus, no self loops allowed!
					if (currentSCC != succSCC) {

						/*
						 * Notice that the edge goes from the successor to the predecessor.
						 */
						dagSCC[succSCC].insert(currentSCC);
					}

				}

			}

		}

		assert (localSCCs.size() == dagSCC.size() && "DAG of SCCs have a wrong number of nodes");

		//Step 2: Compute topological order (greedy algorithm)
		while ( dagSCC.size() > 0  ) {

			int currentNode = -1;

			/*
			 * Here we get the first node without predecessors
			 */
			std::map<int, std::set<int> >::iterator it, it_end;
			for(it = dagSCC.begin(), it_end = dagSCC.end(); it != it_end; it++){
				if (it->second.size() == 0) {
					currentNode = it->first;
					break;
				}
			}

			assert(currentNode>=0 && "DAG of SCCs have a cycle (it is not a valid DAG)!");

			topologicalOrderedSCCs.push_back(currentNode);

			/*
			 * Here we remove this SCC from our DAG
			 */
			for(it = dagSCC.begin(), it_end = dagSCC.end(); it != it_end; it++){
				if (it->second.count(currentNode)) {
					it->second.erase(currentNode);
				}
			}
			dagSCC.erase(currentNode);
		}

	}

	return topologicalOrderedSCCs;
}

int llvm::DepGraph::getSCCID(GraphNode* node) {

	if(!sCCs.size()) {

		//Compute SCCs
		recomputeSCCs();

	}

	//Not in any SCC >>> Problem!!!
	if (!reverseSCCMap.count(node)){
		if (node) errs() << "Requesting SCC ID for invalid node: " << &node << "\n";
		else errs() << "Requesting SCC ID for null node pointer\n";
		return -1;
	}

	return reverseSCCMap[node];

}

std::set<GraphNode*> llvm::DepGraph::getSCC(int ID) {

	if(!sCCs.size()) {

		//Compute SCCs
		recomputeSCCs();

	}

	std::set<GraphNode*> result;
	if (sCCs.count(ID)) result = sCCs[ID];
	else errs() << "Requesting SCC for invalid ID: " << ID <<"\n";

	return result;

}




/*
 * getAcyclicPaths - Returns a list of acyclic paths (hamiltonian paths)
 * from a source node to a destination node.
 *
 * This method implements a modified breadth-first search to collect the
 * paths using a backtracking strategy
 */
void llvm::DepGraph::getAcyclicPaths_rec(
		                 GraphNode* dst,
		                 std::set<GraphNode*> &visitedNodes,
		                 std::stack<GraphNode*> &path,
		                 std::set<std::stack<GraphNode*> > &result,
		                 int SCCID) {

	GraphNode* u = path.top();

	std::map<GraphNode*, edgeType> succs = u->getSuccessors();
	for (std::map<GraphNode*, edgeType>::iterator succ = succs.begin(), s_end =
					succs.end(); succ != s_end; succ++) {

		if(succ->first == dst){
			path.push(dst);

			result.insert(path);

			//truncating number of paths
			if (result.size() >= 1000) return;

			//errs() << "SCC:	"<< SCCID<<"	New Path:" << result.size() << "\n";
			path.pop();
			break;
		}
	}

	if (result.size() >= 1000) return;

	// in breadth-first, recursion needs to come after visiting adjacent nodes
	for (std::map<GraphNode*, edgeType>::iterator succ = succs.begin(), s_end =
					succs.end(); succ != s_end; succ++) {

		if (result.size() >= 1000) return;

		if (visitedNodes.count(succ->first) != 0 && succ->first != dst) continue;


		if (SCCID == -1 || SCCID == getSCCID(succ->first)) {

			//Pruning
			if (!acyclicPathExists(succ->first, dst, visitedNodes, SCCID)) continue;


			visitedNodes.insert(succ->first);
			path.push(succ->first);

			getAcyclicPaths_rec(dst, visitedNodes, path, result, SCCID);

			//truncating number of paths
			if (result.size() >= 1000) return;

			visitedNodes.erase(succ->first);
			path.pop();

		}
	}
}


std::set<std::stack<GraphNode*> > llvm::DepGraph::getAcyclicPaths(GraphNode* src,
		GraphNode* dst) {

	std::set<std::stack<GraphNode*> > result;

	std::set<GraphNode*> visitedNodes;
	std::stack<GraphNode*> path;

	visitedNodes.insert(src);
	path.push(src);

	getAcyclicPaths_rec(dst, visitedNodes, path, result, -1);

	return result;


}


std::set<std::stack<GraphNode*> > llvm::DepGraph::getAcyclicPathsInsideSCC(GraphNode* src, GraphNode* dst){


	std::set<std::stack<GraphNode*> > result;

	std::set<GraphNode*> visitedNodes;
	std::stack<GraphNode*> path;

	visitedNodes.insert(src);
	path.push(src);

	int SCCID = getSCCID(src);

	getAcyclicPaths_rec(dst, visitedNodes, path, result, SCCID);

	return result;
}

bool DepGraph::acyclicPathExists(GraphNode* src,
		GraphNode* dst,
        std::set<GraphNode*> visitedNodes,
        int SCCID){

	std::set<GraphNode*> worklist;
	worklist.insert(src);

	while(worklist.size()){

		GraphNode* current = *worklist.begin();
		worklist.erase(current);

		if (current==dst) return true;

		if (visitedNodes.count(current)) {

			//Item that have just been popped from the worklist and have already been visited
			errs() << "ERROR! "<< current->getLabel() <<"\n";
			continue;

		}
		visitedNodes.insert(current);

	    std::map<GraphNode*, edgeType> succs = current->getSuccessors();

	    for (std::map<GraphNode*, edgeType>::iterator succ = succs.begin(), s_end =
	                    succs.end(); succ != s_end; succ++) {


	    	if (visitedNodes.count(succ->first) && succ->first != dst) continue;
	    	if (worklist.count(succ->first)) continue;


			if(SCCID == -1 || SCCID == getSCCID(succ->first)){

				//Only insert in the worklist items that have not been visited yet
				worklist.insert(succ->first);

			}
	    }
	}

	return false;

}

//*********************************************************************************************************************************************************************
//                                                                                                                              DEPENDENCE GRAPH CLIENT
//*********************************************************************************************************************************************************************
//vector
// Author: Raphael E. Rodrigues
// Contact: raphaelernani@gmail.com
// Date: 05/03/2013
// Last update: 05/03/2013
// Project: e-CoSoc (Intel and Computer Science department of Federal University of Minas Gerais)
//
//*********************************************************************************************************************************************************************


//Class functionDepGraph
void functionDepGraph::getAnalysisUsage(AnalysisUsage &AU) const {
	if (USE_ALIAS_SETS) AU.addRequired<AliasSets> ();
	AU.setPreservesAll();
}

bool functionDepGraph::runOnFunction(Function &F) {

        AliasSets* AS = NULL;

        if (USE_ALIAS_SETS)
                AS = &(getAnalysis<AliasSets> ());

        //Making dependency graph
        depGraph = new DepGraph(AS);
        //Insert instructions in the graph
        for (Function::iterator BBit = F.begin(), BBend = F.end(); BBit != BBend; ++BBit) {
                for (BasicBlock::iterator Iit = BBit->begin(), Iend = BBit->end(); Iit
                                != Iend; ++Iit) {
                        depGraph->addInst(Iit);
                }
        }

        //We don't modify anything, so we must return false
        return false;
}


char functionDepGraph::ID = 0;
static RegisterPass<functionDepGraph> X("functionDepGraph",
                "Function Dependence Graph");

//Class moduleDepGraph
void moduleDepGraph::getAnalysisUsage(AnalysisUsage &AU) const {

	if (USE_ALIAS_SETS)	AU.addRequired<AliasSets> ();

	AU.setPreservesAll();
}

bool moduleDepGraph::runOnModule(Module &M) {

        AliasSets* AS = NULL;

        if (USE_ALIAS_SETS)
                AS = &(getAnalysis<AliasSets> ());

        //Making dependency graph
        depGraph = new DepGraph(AS);

        //Insert instructions in the graph
        for (Module::iterator Fit = M.begin(), Fend = M.end(); Fit != Fend; ++Fit) {
                for (Function::iterator BBit = Fit->begin(), BBend = Fit->end(); BBit
                                != BBend; ++BBit) {
                        for (BasicBlock::iterator Iit = BBit->begin(), Iend = BBit->end(); Iit
                                        != Iend; ++Iit) {
                                depGraph->addInst(Iit);
                        }
                }
        }

        //Connect formal and actual parameters and return values
        for (Module::iterator Fit = M.begin(), Fend = M.end(); Fit != Fend; ++Fit) {

                // If the function is empty, do not do anything
                // Empty functions include externally linked ones (i.e. abort, printf, scanf, ...)
                if (Fit->begin() == Fit->end())
                        continue;

                matchParametersAndReturnValues(*Fit);

        }

        //We don't modify anything, so we must return false
        return false;
}

void moduleDepGraph::matchParametersAndReturnValues(Function &F) {

        // Only do the matching if F has any use
        if (F.isVarArg() || !F.hasNUsesOrMore(1)) {
                return;
        }

        // Data structure which contains the matches between formal and real parameters
        // First: formal parameter
        // Second: real parameter
        SmallVector<std::pair<GraphNode*, GraphNode*>, 4> Parameters(F.arg_size());

        // Fetch the function arguments (formal parameters) into the data structure
        Function::arg_iterator argptr;
        Function::arg_iterator e;
        unsigned i;

        //Create the PHI nodes for the formal parameters
        for (i = 0, argptr = F.arg_begin(), e = F.arg_end(); argptr != e; ++i, ++argptr) {

                OpNode* argPHI = new OpNode(Instruction::PHI);
                GraphNode* argNode = NULL;
                argNode = depGraph->addInst(argptr);

                if (argNode != NULL)
                        depGraph->addEdge(argPHI, argNode);

                Parameters[i].first = argPHI;
        }

        // Check if the function returns a supported value type. If not, no return value matching is done
        bool noReturn = F.getReturnType()->isVoidTy();

        // Creates the data structure which receives the return values of the function, if there is any
        SmallPtrSet<llvm::Value*, 8> ReturnValues;

        if (!noReturn) {
                // Iterate over the basic blocks to fetch all possible return values
                for (Function::iterator bb = F.begin(), bbend = F.end(); bb != bbend; ++bb) {
                        // Get the terminator instruction of the basic block and check if it's
                        // a return instruction: if it's not, continue to next basic block
                        Instruction *terminator = bb->getTerminator();

                        ReturnInst *RI = dyn_cast<ReturnInst> (terminator);

                        if (!RI)
                                continue;

                        // Get the return value and insert in the data structure
                        ReturnValues.insert(RI->getReturnValue());
                }
        }

        for (Value::use_iterator UI = F.use_begin(), E = F.use_end(); UI != E; ++UI) {
                User *U = *UI;

                // Ignore blockaddress uses
                if (isa<BlockAddress> (U))
                        continue;

                // Used by a non-instruction, or not the callee of a function, do not
                // match.
                if (!isa<CallInst> (U) && !isa<InvokeInst> (U))
                        continue;

                Instruction *caller = cast<Instruction> (U);

                CallSite CS(caller);
                if (!CS.isCallee(UI))
                        continue;

                // Iterate over the real parameters and put them in the data structure
                CallSite::arg_iterator AI;
                CallSite::arg_iterator EI;

                for (i = 0, AI = CS.arg_begin(), EI = CS.arg_end(); AI != EI; ++i, ++AI) {
                        Parameters[i].second = depGraph->addInst(*AI);
                }

                // Match formal and real parameters
                for (i = 0; i < Parameters.size(); ++i) {

                        depGraph->addEdge(Parameters[i].second, Parameters[i].first);
                }

                // Match return values
                if (!noReturn) {

                        OpNode* retPHI = new OpNode(Instruction::PHI);
                        GraphNode* callerNode = depGraph->addInst(caller);
                        depGraph->addEdge(retPHI, callerNode);

                        for (SmallPtrSetIterator<llvm::Value*> ri = ReturnValues.begin(),
                                        re = ReturnValues.end(); ri != re; ++ri) {
                                GraphNode* retNode = depGraph->addInst(*ri);
                                depGraph->addEdge(retNode, retPHI);
                        }

                }

                // Real parameters are cleaned before moving to the next use (for safety's sake)
                for (i = 0; i < Parameters.size(); ++i)
                        Parameters[i].second = NULL;
        }

        depGraph->deleteCallNodes(&F);
}

void llvm::moduleDepGraph::deleteCallNodes(Function* F) {
        depGraph->deleteCallNodes(F);
}

void llvm::DepGraph::Guider::setNodeAttrs(GraphNode* n, std::string attrs) {
        nodeAttrs[n] = attrs;
}

void llvm::DepGraph::Guider::setEdgeAttrs(GraphNode* u, GraphNode* v,
                std::string attrs) {
        edgeAttrs[std::pair<GraphNode*, GraphNode*>(u, v)] = attrs;
}

void llvm::DepGraph::Guider::clear() {
        nodeAttrs.clear();
        edgeAttrs.clear();
}

std::string llvm::DepGraph::Guider::getNodeAttrs(GraphNode* n) {
        return nodeAttrs[n];
}

std::string llvm::DepGraph::Guider::getEdgeAttrs(GraphNode* u, GraphNode* v) {
        return edgeAttrs[std::pair<GraphNode*, GraphNode*>(u, v)];
}

char moduleDepGraph::ID = 0;
static RegisterPass<moduleDepGraph> Y("moduleDepGraph",
                "Module Dependence Graph");

char ViewModuleDepGraph::ID = 0;
static RegisterPass<ViewModuleDepGraph> Z("view-depgraph",
		"View Module Dependence Graph");


