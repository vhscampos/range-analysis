#include "GenericGraph.h"

//*********************************************************************************************************************************************************************
//                                                                                                                              DEPENDENCE GRAPH API
//*********************************************************************************************************************************************************************
//
// Author: Raphael E. Rodrigues
// Contact: raphaelernani@gmail.com
// Date: 16/01/2014
// Institution: Computer Science department of Federal University of Minas Gerais
//
//*********************************************************************************************************************************************************************


/*
 * Class GenericGraphNode
 */

template <typename T>
GenericGraphNode<T>::GenericGraphNode(T obj) {
    element = obj;
	ID = currentID++;
}

template <typename T>
GenericGraphNode<T>::~GenericGraphNode() {

        for (typename std::map<GenericGraphNode<T>*, int>::iterator pred = predecessors.begin(); pred
                        != predecessors.end(); pred++) {

        	(*pred).first->successors.erase(this);
        }

        for (typename std::map<GenericGraphNode<T>*, int>::iterator succ = successors.begin(); succ
                        != successors.end(); succ++) {
                (*succ).first->predecessors.erase(this);
        }

        successors.clear();
        predecessors.clear();
}

template <typename T>
std::map<GenericGraphNode<T>*, int> GenericGraphNode<T>::getSuccessors() {
        return successors;
}

template <typename T>
std::map<GenericGraphNode<T>*, int> GenericGraphNode<T>::getPredecessors() {
        return predecessors;
}

template <typename T>
void GenericGraphNode<T>::connect(GenericGraphNode<T>* dst, int weight) {

        this->successors[dst] = weight;
        dst->predecessors[this] = weight;

}

template <typename T>
void GenericGraphNode<T>::disconnect(GenericGraphNode<T>* dst) {

    if (this->successors.find(dst) != this->successors.end()) {
    	this->successors.erase(dst);
    	dst->predecessors.erase(this);
    }
}

template <typename T>
int GenericGraphNode<T>::getId() const {
        return ID;
}

template <typename T>
bool GenericGraphNode<T>::hasSuccessor(GenericGraphNode<T>* succ) {
        return successors.count(succ) > 0;
}


template <typename T>
bool GenericGraphNode<T>::hasPredecessor(GenericGraphNode<T>* pred) {
        return predecessors.count(pred) > 0;
}

template <typename T>
std::string GenericGraphNode<T>::getName() {
        std::ostringstream stringStream;
        stringStream << "node_" << getId();
        return stringStream.str();
}

template <typename T>
std::string GenericGraphNode<T>::getShape() {
	return std::string("ellipse");
}

template <typename T>
std::string GenericGraphNode<T>::getStyle() {
        return std::string("solid");
}

template <typename T>
int GenericGraphNode<T>::currentID = 0;


/*
 * Class GenericGraph
 */
template <typename T>
typename GenericGraph<T>::iterator GenericGraph<T>::begin(){
	return(nodes.begin());
}

template <typename T>
typename GenericGraph<T>::iterator GenericGraph<T>::end(){
	return(nodes.end());
}

template <typename T>
GenericGraph<T>::~GenericGraph() {
	nodes.clear();
}

template <typename T>
void GenericGraph<T>::dfsVisit(GenericGraphNode<T>* u, std::set<GenericGraphNode<T>*> &visitedNodes) {

        visitedNodes.insert(u);

        std::map<GenericGraphNode<T>*, int> succs = u->getSuccessors();

        for (typename std::map<GenericGraphNode<T>*, int>::iterator succ = succs.begin(), s_end =
                        succs.end(); succ != s_end; succ++) {
                if (visitedNodes.count(succ->first) == 0) {
                        dfsVisit(succ->first, visitedNodes);
                }
        }

}


template <typename T>
void GenericGraph<T>::dfsVisitBack(GenericGraphNode<T>* u, std::set<GenericGraphNode<T>*> &visitedNodes) {

        visitedNodes.insert(u);

        std::map<GenericGraphNode<T>*, int> preds = u->getPredecessors();

        for (typename std::map<GenericGraphNode<T>*, int>::iterator pred = preds.begin(), s_end =
                        preds.end(); pred != s_end; pred++) {
                if (visitedNodes.count(pred->first) == 0) {
                        dfsVisitBack(pred->first, visitedNodes);
                }
        }

}


//Print the graph (.dot format) in the stderr stream.
/*
 *
 * TODO: Implement this
 *
 */
//void GenericGraph<T>::toDot(std::string s) {
//
//	c_err << "Sorry, Not Implemented yet\n";
//}
//
//void GenericGraph<T>::toDot(std::string s, const std::string fileName) {
//
//        std::string ErrorInfo;
//
//        raw_fd_ostream File(fileName.c_str(), ErrorInfo);
//
//        if (!ErrorInfo.empty()) {
//                errs() << "Error opening file " << fileName
//                                << " for writing! Error Info: " << ErrorInfo << " \n";
//                return;
//        }
//
//        this->toDot(s, &File);
//
//}
//
//void GenericGraph<T>::toDot(std::string s, raw_ostream *stream) {
//
//        (*stream) << "digraph \"DFG for \'" << s << "\' function \"{\n";
//        (*stream) << "label=\"DFG for \'" << s << "\' function\";\n";
//
//        std::map<GenericGraphNode<T>*, int> DefinedNodes;
//
//        for (std::set<GenericGraphNode<T>*>::iterator node = nodes.begin(), end = nodes.end(); node
//                        != end; node++) {
//
//                if (DefinedNodes.count(*node) == 0) {
//                        (*stream) << (*node)->getName() << "[shape=" << (*node)->getShape()
//                                        << ",style=" << (*node)->getStyle() << ",label=\""
//                                        << (*node)->getLabel() << "\"]\n";
//                        DefinedNodes[*node] = 1;
//                }
//
//                std::map<GenericGraphNode<T>*, int> succs = (*node)->getSuccessors();
//
//                for (std::map<GenericGraphNode<T>*, int>::iterator succ = succs.begin(),
//                                s_end = succs.end(); succ != s_end; succ++) {
//
//                        if (DefinedNodes.count(succ->first) == 0) {
//                                (*stream) << (succ->first)->getName() << "[shape="
//                                                << (succ->first)->getShape() << ",style="
//                                                << (succ->first)->getStyle() << ",label=\""
//                                                << (succ->first)->getLabel() << "\"]\n";
//                                DefinedNodes[succ->first] = 1;
//                        }
//
//                        //Source
//                        (*stream) << "\"" << (*node)->getName() << "\"";
//
//                        (*stream) << "->";
//
//                        //Destination
//                        (*stream) << "\"" << (succ->first)->getName() << "\"";
//
//                        if (succ->second == etControl)
//                                (*stream) << " [style=dashed]";
//
//                        (*stream) << "\n";
//
//                }
//
//        }
//
//        (*stream) << "}\n\n";
//
//}


template <typename T>
void GenericGraph<T>::removeNode(GenericGraphNode<T>* target){
    nodes.erase(target);
    delete target;

}

template <typename T>
GenericGraphNode<T>* GenericGraph<T>::addNode(T element) {

	GenericGraphNode<T>* node = this->findNode(element);

	if (!node) {

		node = new GenericGraphNode<T>(element);

		nodes.insert(node);
		nodeMap[element] = node;
	}

	return node;

}

template <typename T>
void GenericGraph<T>::addEdge(GenericGraphNode<T>* src, GenericGraphNode<T>* dst, int weight) {

        nodes.insert(src);
        nodes.insert(dst);
        src->connect(dst, weight);

}


//Return the pointer to the node related to the element.
//Return NULL if the element is not inside the map.
template <typename T>
GenericGraphNode<T>* GenericGraph<T>::findNode(T element) {

	if (nodeMap.count(element))
		return nodeMap[element];

	return NULL;
}

template <typename T>
GenericGraphNode<T>* GenericGraph<T>::findNode(GenericGraphNode<T>* node) {

		if (nodes.count(node)) return node;

        return NULL;
}

template <typename T>
std::set<GenericGraphNode<T>* > GenericGraph<T>::findNodes(std::set<T> elements) {

        std::set<GenericGraphNode<T>*> result;

        for (typename std::set<T>::iterator i = elements.begin(), end = elements.end(); i != end; i++) {

                if (GenericGraphNode<T>* node = findNode(*i)) {
                        result.insert(node);
                }

        }

        return result;
}

template <typename T>
std::list<GenericGraphNode<T>*> GenericGraph<T>::getNodesWithoutPredecessors() {

	std::list<GenericGraphNode<T>*> result;

    for (typename std::set<GenericGraphNode<T>* >::iterator node = nodes.begin(), end = nodes.end(); node != end; node++) {

             std::map<GenericGraphNode<T>*, int> preds = (*node)->getPredecessors();

             if (preds.size() == 0) result.push_back(*node);

     }

	return result;

}

template <typename T>
GenericGraphNode<T>* GenericGraph<T>::getFirstNodeWithoutPredecessors(){

    for (typename std::set<GenericGraphNode<T>* >::iterator node = nodes.begin(), end = nodes.end(); node != end; node++) {

             std::map<GenericGraphNode<T>*, int> preds = (*node)->getPredecessors();

             if (preds.size() == 0) return *node;

     }

	return NULL;

}

template <typename T>
void GenericGraph<T>::removeEdge(GenericGraphNode<T>* src, GenericGraphNode<T>* dst) {

	src->disconnect(dst);

}

