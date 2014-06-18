/*
 * LoopControllersDepGraph.h
 *
 *  Created on: 24/10/2013
 *      Author: raphael
 */

#ifndef LOOPCONTROLLERSDEPGRAPH_H_
#define LOOPCONTROLLERSDEPGRAPH_H_

#include "DepGraph.h"

namespace llvm {

class LoopControllersDepGraph: public FunctionPass {
private:
	DepGraph* fullGraph;
public:

	DepGraph* depGraph;

	static char ID; // Pass identification, replacement for typeid.
	LoopControllersDepGraph() :
		FunctionPass(ID), depGraph(NULL), fullGraph(NULL) {
	}
	virtual ~LoopControllersDepGraph() {
		freePerspectiveGraph();
	}

	void getAnalysisUsage(AnalysisUsage &AU) const;
	bool runOnFunction(Function& F);

	void freePerspectiveGraph();
	void setPerspective(BasicBlock* LoopHeader);

	void printPers(){
		if (fullGraph == depGraph)	errs() << "Full!\n";
		else errs() << "Pers!	Full:" << fullGraph->getNumVarNodes() << "	Slice:" << depGraph->getNumVarNodes() <<  "\n"   ;
	}

};

class ModuleLoopControllersDepGraph: public ModulePass {
public:
		DepGraph* depGraph;

		static char ID; // Pass identification, replacement for typeid.
		ModuleLoopControllersDepGraph() :
        	ModulePass(ID), depGraph(NULL) {
        }
        void getAnalysisUsage(AnalysisUsage &AU) const;
        bool runOnModule(Module& M);


        void setPerspective(BasicBlock* LoopHeader);

};


class ViewLoopControllersDepGraphSCCs: public FunctionPass {
public:
        static char ID; // Pass identification, replacement for typeid.
        ViewLoopControllersDepGraphSCCs() :
        	FunctionPass(ID) {
        }

        void getAnalysisUsage(AnalysisUsage &AU) const {
                AU.addRequired<LoopControllersDepGraph> ();
                AU.setPreservesAll();
        }

        bool runOnFunction(Function& F) {

        	LoopControllersDepGraph& DepGraphPass = getAnalysis<LoopControllersDepGraph> ();
			DepGraph *g = DepGraphPass.depGraph;


			std::string tmp = F.getParent()->getModuleIdentifier();
			replace(tmp.begin(), tmp.end(), '\\', '_');
			std::string Filename = "/tmp/" + tmp + ".dot";

//			g->toDot(M.getModuleIdentifier(), Filename);
//			DisplayGraph(sys::Path(Filename), true, GraphProgram::DOT);

			std::map<int, std::set<GraphNode*> > SCCs = g->getSCCs();
			for(std::map<int, std::set<GraphNode*> >::iterator it = SCCs.begin(); it!=SCCs.end(); it++){

				errs() << it->first << "\n";

				g->generateSubGraph(it->first).toDot(F.getParent()->getModuleIdentifier(), Filename);
				DisplayGraph(sys::Path(Filename), true, GraphProgram::DOT);
			}

			return false;
        }
};




} /* namespace llvm */
#endif /* LOOPCONTROLLERSDEPGRAPH_H_ */
