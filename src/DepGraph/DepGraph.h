#ifndef DEPGRAPH_H_
#define DEPGRAPH_H_

#ifndef DEBUG_TYPE
#define DEBUG_TYPE "depgraph"
#endif

#define USE_ALIAS_SETS true

#include "llvm/Pass.h"
#include "llvm/PassAnalysisSupport.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/GraphWriter.h"
#include "llvm/Support/raw_ostream.h"
#include "LoopInfoEx.h"
#include "AliasSets.h"
#include "GenericGraph.h"
#include <list>
#include <map>
#include <set>
#include <stack>
#include <sstream>
#include <stdio.h>

using namespace std;

namespace llvm {
STATISTIC(NrOpNodes, "Number of operation nodes");
STATISTIC(NrVarNodes, "Number of variable nodes");
STATISTIC(NrMemNodes, "Number of memory nodes");
STATISTIC(NrEdges, "Number of edges");

typedef enum {
        etData = 0, etControl = 1
} edgeType;

typedef enum {
	GraphNodeId,
	VarNodeId,
	OpNodeId,
	CallNodeId,
	MemNodeId,
	BinaryOpNodeId,
	UnaryOpNodeId,
	SigmaOpNodeId,
	PHIOpNodeId
} NodeClassId;

/*
 * Class GraphNode
 *
 * This abstract class can do everything a simple graph node can do:
 *              - It knows the nodes that points to it
 *              - It knows the nodes who are ponted by it
 *              - It has a unique ID that can be used to identify the node
 *              - It knows how to connect itself to another GraphNode
 *
 * This class provides virtual methods that makes possible printing the graph
 * in a fancy .dot file, providing for each node:
 *              - Label
 *              - Shape
 *              - Style
 *
 */
class GraphNode {
private:
        static int currentID;
        int ID;

protected:
        NodeClassId Class_ID;
        std::map<GraphNode*, edgeType> successors;
        std::map<GraphNode*, edgeType> predecessors;
public:
        GraphNode();
        GraphNode(GraphNode &G);

        virtual ~GraphNode();

        static inline bool classof(const GraphNode *N) {
                return true;
        }
        ;
        std::map<GraphNode*, edgeType> getSuccessors();
        bool hasSuccessor(GraphNode* succ);

        std::map<GraphNode*, edgeType> getPredecessors();
        bool hasPredecessor(GraphNode* pred);

        void connect(GraphNode* dst, edgeType type = etData);
        void disconnect(GraphNode* dst);

        NodeClassId getClass_Id() const;
        int getId() const;
        std::string getName();
        virtual std::string getLabel() = 0;
        virtual std::string getShape() = 0;
        virtual std::string getStyle();

        virtual llvm::raw_ostream& dump(llvm::raw_ostream &strm);

        static std::string getClassName(NodeClassId classID);

        virtual GraphNode* clone() = 0;
};

//extern llvm::raw_ostream& operator<<(llvm::raw_ostream &strm, GraphNode &a);

//llvm::raw_ostream& operator<<(llvm::raw_ostream &strm, GraphNode &a) {
//  return a.dump(strm);
//}

/*
 * Class OpNode
 *
 * This class represents the operation nodes:
 *              - It has a OpCode that is compatible with llvm::Instruction OpCodes
 *              - It may or may not store a value, that is the variable defined by the operation
 */
class OpNode: public GraphNode {
private:
        unsigned int OpCode;
        Instruction* inst;
public:
        OpNode(int OpCode) :
                GraphNode(), OpCode(OpCode), inst(NULL) {
                this->Class_ID = OpNodeId;
                NrOpNodes++;
        }
        ;
        OpNode(Instruction* i) :
                GraphNode(), OpCode(i->getOpcode()), inst(i) {
                this->Class_ID = OpNodeId;
                NrOpNodes++;
        }
        ;
        ~OpNode() {
                NrOpNodes--;
        }
        ;
        static inline bool classof(const GraphNode *N) {
                return N->getClass_Id() == OpNodeId
                	|| N->getClass_Id() == CallNodeId
                	|| N->getClass_Id() == PHIOpNodeId
                	|| N->getClass_Id() == BinaryOpNodeId
                	|| N->getClass_Id() == UnaryOpNodeId
                	|| N->getClass_Id() == SigmaOpNodeId;
        }
        ;
        unsigned int getOpCode() const;
        void setOpCode(unsigned int opCode);
        Instruction* getOperation() {return inst;};

        virtual GraphNode* getIncomingNode(unsigned int index = 0, edgeType et = etData);
        virtual GraphNode* getOperand(unsigned int index);

        virtual std::string getLabel();
        virtual std::string getShape();

        GraphNode* clone();
};


/*
 * Class UnaryOpNode
 *
 * This class represents operation nodes of llvm::UnaryInstruction instructions
 */
class UnaryOpNode: public OpNode {
private:
	UnaryInstruction* UOP;
public:
	UnaryOpNode(UnaryInstruction* UOP) :
                OpNode(UOP), UOP(UOP) {
                this->Class_ID = UnaryOpNodeId;
        }
        ;
	static inline bool classof(const GraphNode *N) {
			return N->getClass_Id() == UnaryOpNodeId
				|| N->getClass_Id() == SigmaOpNodeId;
	}
	;

	UnaryInstruction* getUnaryInstruction() {return UOP;};

	GraphNode* clone();
};

/*
 * Class SigmaOpNode
 *
 * This class represents operation nodes of llvm::PHINode instructions that are sigma operations
 */
class SigmaOpNode: public OpNode {
private:
        PHINode* Sigma;
public:
        SigmaOpNode(PHINode* Sigma) :
        		OpNode(Sigma), Sigma(Sigma) {
                this->Class_ID = SigmaOpNodeId;
        }
        ;
        static inline bool classof(const GraphNode *N) {
                return N->getClass_Id() == SigmaOpNodeId;
        }
        ;
        PHINode* getSigma() {return Sigma;};

        std::string getLabel();
        std::string getShape();
        std::string getStyle();

        GraphNode* clone();
};


/*
 * Class PHIOpNode
 *
 * This class represents operation nodes of llvm::PHINode instructions
 */
class PHIOpNode: public OpNode {
private:
        PHINode* PHI;
public:
        PHIOpNode(PHINode* PHI) :
                OpNode(PHI), PHI(PHI) {
                this->Class_ID = PHIOpNodeId;
        }
        ;
        static inline bool classof(const GraphNode *N) {
                return N->getClass_Id() == PHIOpNodeId;
        }
        ;
        PHINode* getPHINode() {return PHI;};

        std::string getLabel();
        std::string getShape();
        std::string getStyle();

        GraphNode* clone();
};

/*
 * Class BinaryOpNode
 *
 * This class represents operation nodes of llvm::BinaryOperator instructions
 */
class BinaryOpNode: public OpNode {
private:
        BinaryOperator* BOP;
public:
        BinaryOpNode(BinaryOperator* BOP) :
                OpNode(BOP), BOP(BOP) {
                this->Class_ID = BinaryOpNodeId;
        }
        ;
        static inline bool classof(const GraphNode *N) {
                return N->getClass_Id() == BinaryOpNodeId;
        }
        ;
        BinaryOperator* getBinaryOperator() {return BOP;};

        GraphNode* clone();
};

/*
 * Class CallNode
 *
 * This class represents operation nodes of llvm::Call instructions:
 *              - It stores the pointer to the called function
 */
class CallNode: public OpNode {
private:
        CallInst* CI;
public:
        CallNode(CallInst* CI) :
                OpNode(CI), CI(CI) {
                this->Class_ID = CallNodeId;
        }
        ;
        static inline bool classof(const GraphNode *N) {
                return N->getClass_Id() == CallNodeId;
        }
        ;
        Function* getCalledFunction() const;

        CallInst* getCallInst() const;

        std::string getLabel();
        std::string getShape();

        GraphNode* clone();
};

/*
 * Class VarNode
 *
 * This class represents variables and constants which are not pointers:
 *              - It stores the pointer to the corresponding Value*
 */
class VarNode: public GraphNode {
private:
        Value* value;
public:
        VarNode(Value* value) :
                GraphNode(), value(value) {
                this->Class_ID = VarNodeId;
                NrVarNodes++;
        }
        ;
        ~VarNode() {
                NrVarNodes--;
        }
        static inline bool classof(const GraphNode *N) {
                return N->getClass_Id() == VarNodeId;
        }
        ;
        Value* getValue();

        std::string getLabel();
        std::string getShape();

        GraphNode* clone();
};




/*
 * Class MemNode
 *
 * This class represents memory as AliasSets of pointer values:
 *              - It stores the ID of the AliasSet
 *              - It provides a method to get access to all the Values contained in the AliasSet
 */
class MemNode: public GraphNode {
private:
        int aliasSetID;
        AliasSets *AS;
public:
        MemNode(int aliasSetID, AliasSets *AS) :
                aliasSetID(aliasSetID), AS(AS) {
                this->Class_ID = MemNodeId;
                NrMemNodes++;
        }
        ;
        ~MemNode() {
                NrMemNodes--;
        }
        ;
        static inline bool classof(const GraphNode *N) {
                return N->getClass_Id() == MemNodeId;
        }
        ;
        std::set<Value*> getAliases();

        std::string getLabel();
        std::string getShape();
        GraphNode* clone();
        std::string getStyle();

        int getAliasSetId() const;
};





/*
 * Class Graph
 *
 * Stores a set of nodes. Each node knows how to go to other nodes.
 *
 * The class provides methods to:
 *              - Find specific nodes
 *              - Delete specific nodes
 *              - Print the graph
 *
 */
//Dependence Graph
class DepGraph {
private:
		//Graph nodes
		std::set<GraphNode*> nodes;							    //List of nodes of the graph
		llvm::DenseMap<const Value*, GraphNode*> opNodes;		//Subset of nodes
        llvm::DenseMap<const Value*, GraphNode*> callNodes;		//Subset of opnodes
        llvm::DenseMap<const Value*, GraphNode*> varNodes;		//Subset of nodes
        llvm::DenseMap<int, GraphNode*> memNodes;			    //Subset of nodes

		//Navigation through subgraphs
		DepGraph* parentGraph;							//Graph that has originated this graph
		std::map<GraphNode*, GraphNode*> nodeMap;	//Correspondence of nodes between graphs

		//Graph analysis - Strongly connected components
		std::map<int, std::set<GraphNode*> > sCCs;
		std::map<GraphNode*, int> reverseSCCMap;
		std::list<int> topologicalOrderedSCCs;

		bool isSigma(const PHINode* Phi);

		AliasSets *AS;

        bool isValidInst(const Value *v); //Return true if the instruction is valid for dependence graph construction
        bool isMemoryPointer(const Value *v); //Return true if the value is a memory pointer

public:
        typedef std::set<GraphNode*>::iterator iterator;

        std::set<GraphNode*>::iterator begin();
        std::set<GraphNode*>::iterator end();

        DepGraph(AliasSets *AS) :
        	parentGraph(NULL), AS(AS){
            NrEdges = 0;
        }
        ; //Constructor
        ~DepGraph(); //Destructor - Free adjacent matrix's memory
        GraphNode* addInst(Value *v); //Add an instruction into Dependence Graph

        void removeNode(GraphNode* target);

        void addEdge(GraphNode* src, GraphNode* dst, edgeType type = etData);
        void removeEdge(GraphNode* src, GraphNode* dst);

        GraphNode* findNode(const Value *op); //Return the pointer to the node or NULL if it is not in the graph
        GraphNode* findNode(GraphNode* node); //Return the pointer to the node or NULL if it is not in the graph

        std::set<GraphNode*> findNodes(std::set<Value*> values);

        OpNode* findOpNode(const Value *op); //Return the pointer to the node or NULL if it is not in the graph

        //print graph in dot format
        class Guider {
        public:
                std::string getNodeAttrs(GraphNode* n);
                std::string getEdgeAttrs(GraphNode* u, GraphNode* v);
                void setNodeAttrs(GraphNode* n, std::string attrs);
                void setEdgeAttrs(GraphNode* u, GraphNode* v, std::string attrs);
                void clear();
        private:
                DenseMap<GraphNode*, std::string> nodeAttrs;
                DenseMap<std::pair<GraphNode*, GraphNode*>, std::string> edgeAttrs;
        };

        void toDot(std::string s); //print in stdErr
        void toDot(std::string s, std::string fileName); //print in a file
        void toDot(std::string s, raw_ostream *stream); //print in any stream
        void toDot(std::string s, raw_ostream *stream, llvm::DepGraph::Guider* g);



        //Creates an entirely new graph, with equivalent nodes and edges
        DepGraph* clone();

        DepGraph makeSubGraph(std::set<GraphNode*> nodeList);

        DepGraph generateSubGraph(Value *src, Value *dst); //Take a source value and a destination value and find a Connecting Subgraph from source to destination
        DepGraph generateSubGraph(GraphNode* src, GraphNode* dst);
        DepGraph generateSubGraph(int SCCID); //Generate sub graph containing only the selected SCC



        void dfsVisit(GraphNode* u, std::set<GraphNode*> &visitedNodes); //Used by findConnectingSubgraph() method
        void dfsVisitBack(GraphNode* u, std::set<GraphNode*> &visitedNodes); //Used by findConnectingSubgraph() method

        void dfsVisitBack_ext(GraphNode* u, std::set<GraphNode*> &visitedNodes, std::map<int, GraphNode*> &firstNodeVisitedPerSCC);



        void deleteCallNodes(Function* F);

        /*
         * Function getNearestDependence
         *
         * Given a sink, returns the nearest source in the graph and the distance to the nearest source
         */
        std::pair<GraphNode*, int> getNearestDependency(Value* sink,
                        std::set<Value*> sources, bool skipMemoryNodes);

        /*
         * Function getEveryDependency
         *
         * Given a sink, returns shortest path to each source (if it exists)
         */
        std::map<GraphNode*, std::vector<GraphNode*> > getEveryDependency(
                        llvm::Value* sink, std::set<llvm::Value*> sources,
                        bool skipMemoryNodes);


        int getNumOpNodes();
        int getNumCallNodes();
        int getNumMemNodes();
        int getNumVarNodes();
        int getNumDataEdges();
        int getNumControlEdges();
        int getNumEdges(edgeType type);

        std::list<GraphNode*> getNodesWithoutPredecessors();


        DepGraph* getParentGraph();

        void strongconnect(GraphNode* node,
        		           std::map<GraphNode*, int> &index,
        		           std::map<GraphNode*, int> &lowlink,
        		           int &currentIndex,
        		           std::stack<GraphNode*> &S,
        		           std::set<GraphNode*> &S2,
        		           std::map<int, std::set<GraphNode*> > &SCCs);


        void recomputeSCCs();
        std::map<int, std::set<GraphNode*> > getSCCs();
        std::list<int> getSCCTopologicalOrder();

        int getSCCID(GraphNode* node);
        std::set<GraphNode*> getSCC(int ID);

        void printSCC(int SCCid, raw_ostream& OS);

        void dumpSCCs();

        bool acyclicPathExists(GraphNode* src,
        		GraphNode* dst,
                std::set<GraphNode*> alreadyVisitedNodes,
                int SCCID);

        bool hasNestedLoop(int SCCID);
        bool hasNestedLoop(GraphNode* first);

        void getAcyclicPaths_rec(GraphNode* dst,
        		                 std::set<GraphNode*> &visitedNodes,
        		                 std::stack<GraphNode*> &path,
        		                 std::set<std::stack<GraphNode*> > &result,
        		                 int SCCID);

        std::set<std::stack<GraphNode*> > getAcyclicPaths(GraphNode* src, GraphNode* dst);
        std::set<std::stack<GraphNode*> > getAcyclicPathsInsideSCC(GraphNode* src, GraphNode* dst);

};

//TODO: Refactor this
class SCC_Iterator {
private:
	DepGraph* Graph;
	int SCCID;

	DepGraph::iterator currentIt;

public:

	SCC_Iterator(DepGraph* Graph, int SCCID): Graph(Graph), SCCID(SCCID){

		for( DepGraph::iterator node_it = Graph->begin(), node_end = Graph->end();  node_it != node_end; node_it++ ){
			GraphNode* current_node = *node_it;
			if(Graph->getSCCID(current_node) == SCCID){
				currentIt = node_it;
				break;
			}
		}

	};
	~SCC_Iterator(){};

	bool hasNext(){
		return (currentIt != Graph->end());
	}

	GraphNode* getNext(){

		if (!hasNext()) return NULL;
		GraphNode* result = *currentIt;
		currentIt++;

		for(DepGraph::iterator node_end = Graph->end();  currentIt != node_end; currentIt++ ){
			GraphNode* current_node = *currentIt;
			if(Graph->getSCCID(current_node) == SCCID){
				break;
			}
		}

		return result;
	}



};


/*
 * Class functionDepGraph
 *
 * Function pass that provides an intraprocedural dependency graph
 *
 */
class functionDepGraph: public FunctionPass {
public:
        static char ID; // Pass identification, replacement for typeid.
        functionDepGraph() :
                FunctionPass(ID), depGraph(NULL) {
        }
        void getAnalysisUsage(AnalysisUsage &AU) const;
        bool runOnFunction(Function&);

        DepGraph* depGraph;
};

/*
 * Class moduleDepGraph
 *
 * Module pass that provides a context-insensitive interprocedural dependence graph
 *
 */
class moduleDepGraph: public ModulePass {
public:
        static char ID; // Pass identification, replacement for typeid.
        moduleDepGraph() :
                ModulePass(ID), depGraph(NULL) {
        }
        void getAnalysisUsage(AnalysisUsage &AU) const;
        bool runOnModule(Module&);

        void matchParametersAndReturnValues(Function &F);
        void deleteCallNodes(Function* F);

        DepGraph* depGraph;
};


class ViewModuleDepGraph: public ModulePass {
public:
        static char ID; // Pass identification, replacement for typeid.
        ViewModuleDepGraph() :
                ModulePass(ID) {
        }

        void getAnalysisUsage(AnalysisUsage &AU) const {
                AU.addRequired<moduleDepGraph> ();
                AU.setPreservesAll();
        }

        bool runOnModule(Module& M) {

                moduleDepGraph& DepGraphPass = getAnalysis<moduleDepGraph> ();
                DepGraph *graph = DepGraphPass.depGraph;

                std::string tmp = M.getModuleIdentifier();
                replace(tmp.begin(), tmp.end(), '\\', '_');

                std::string Filename = "/tmp/" + tmp + ".dot";

                //Print dependency graph (in dot format)
                graph->toDot(M.getModuleIdentifier(), Filename);

                DisplayGraph(sys::Path(Filename), true, GraphProgram::DOT);

                return false;
        }
};

bool isSigma(const PHINode* Phi);



}

#endif //DEPGRAPH_H_
