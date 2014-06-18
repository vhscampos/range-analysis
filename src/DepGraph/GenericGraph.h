#ifndef GENERICDEPGRAPH_H_
#define GENERICDEPGRAPH_H_


#include <list>
#include <map>
#include <set>
#include <stack>
#include <sstream>
#include <stdio.h>

using namespace std;

/*
 * Class GenericGraphNode
 *
 * This abstract class can do everything a simple graph node can do:
 *              - It knows the nodes that points to it
 *              - It knows the nodes who are ponted by it
 *              - It has a unique ID that can be used to identify the node
 *              - It knows how to connect itself to another GenericGraphNode
 *
 * This class provides virtual methods that makes possible printing the graph
 * in a .dot file, providing for each node:
 *              - Label
 *              - Shape
 *              - Style
 *
 */
template <typename T>
class GenericGraphNode {
private:
        std::map<GenericGraphNode<T>*, int> successors;
        std::map<GenericGraphNode<T>*, int> predecessors;

        T element;

        static int currentID;
        int ID;

public:
        GenericGraphNode(T obj);
        virtual ~GenericGraphNode();

        std::map<GenericGraphNode<T>*, int> getSuccessors();
        bool hasSuccessor(GenericGraphNode<T>* succ);

        std::map<GenericGraphNode<T>*, int> getPredecessors();
        bool hasPredecessor(GenericGraphNode<T>* pred);

        void connect(GenericGraphNode<T>* dst, int weight);
        void disconnect(GenericGraphNode<T>* dst);

        int getId() const;
        std::string getName();
        virtual std::string getShape();
        virtual std::string getStyle();

        inline T operator*(){ return element; };
};


/*
 * Class GenericGraph
 *
 * Stores a set of nodes. Each node knows how to go to other nodes.
 *
 * The class provides methods to:
 *              - Find specific nodes
 *              - Delete specific nodes
 *              - Print the graph
 *
 */
template <typename T>
class GenericGraph {
private:
		//Graph nodes
		std::set<GenericGraphNode<T>*> nodes;	//List of nodes of the graph

		//lookup
		std::map<T,GenericGraphNode<T>* > nodeMap;
public:
        typedef typename std::set<GenericGraphNode<T>*>::iterator iterator;

        iterator begin();
        iterator end();

        GenericGraph() {};
        ~GenericGraph(); //Destructor - Free adjacent matrix's memory
        GenericGraphNode<T>* addNode(T element); //Add an instruction into Dependence Graph

        void removeNode(GenericGraphNode<T>* target);

        void addEdge(GenericGraphNode<T>* src, GenericGraphNode<T>* dst, int weight);
        void removeEdge(GenericGraphNode<T>* src, GenericGraphNode<T>* dst);

        GenericGraphNode<T>* findNode(T element); //Return the pointer to the node or NULL if it is not in the graph
        GenericGraphNode<T>* findNode(GenericGraphNode<T>* node); //Return the pointer to the node or NULL if it is not in the graph

        std::set<GenericGraphNode<T>*> findNodes(std::set<T> elements);

//        void toDot(std::string s); //print in stdErr
//        void toDot(std::string s, std::string fileName); //print in a file

        void dfsVisit(GenericGraphNode<T>* u, std::set<GenericGraphNode<T>*> &visitedNodes); //Used by findConnectingSubgraph() method
        void dfsVisitBack(GenericGraphNode<T>* u, std::set<GenericGraphNode<T>*> &visitedNodes); //Used by findConnectingSubgraph() method

        std::list<GenericGraphNode<T>*> getNodesWithoutPredecessors();
        GenericGraphNode<T>* getFirstNodeWithoutPredecessors();

        inline int getNumNodes() { return nodes.size();};

};


#endif //GENERICDEPGRAPH_H_
