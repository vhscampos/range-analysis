/*
 * ASSegPropagation.cpp
 *
 *  Created on: Jun 5, 2014
 *      Author: raphael
 */
#include "ASSegPropagation.h"

STATISTIC(NumSends, "The number of sends");
STATISTIC(NumRecvs, "The number of recvs");
STATISTIC(NumNetEdges, "The number of network edges");
STATISTIC(NumInsts, "The number of instructions");
STATISTIC(NumPropSegments, "The number of propagated segments");
STATISTIC(NumNonTrivPropSegments,
		"The number of non-trivial propagated segments");
STATISTIC(NumHitQueries, "The number of pointer queries that yield a range");

using namespace llvm;

bool ASSegPropagation::runOnModule(Module &M) {

	//Here we compute the propagation of the big tables

	int propagatedP1 = 0;
	int propagatedP2 = 0;
	long numInsts = 0;

	AliasSetSegmentation &ass = getAnalysis<AliasSetSegmentation>();
	netDepGraph &ndg = getAnalysis<netDepGraph>();
	Graph *dg = ndg.depGraph;

	for (Module::iterator F = M.begin(), Fend = M.end(); F != Fend; ++F) {
		for (Function::iterator BB = F->begin(), BBend = F->end(); BB != BBend;
				++BB) {
			numInsts += (long) (*BB).size();
		}
	}

	NumInsts += numInsts;

	std::set<llvm::GraphNode*> sendP1 = ndg.depGraph->getNetWriteNodes(1);
	std::set<llvm::GraphNode*> sendP2 = ndg.depGraph->getNetWriteNodes(2);
	std::set<llvm::GraphNode*> recvP1 = ndg.depGraph->getNetReadNodes(1);
	std::set<llvm::GraphNode*> recvP2 = ndg.depGraph->getNetReadNodes(2);

	std::map<GraphNode*, SegmentationTable*> sendP1Table;
	std::map<GraphNode*, SegmentationTable*> sendP2Table;

	NumSends = sendP1.size() + sendP2.size();
	NumRecvs = recvP1.size() + recvP2.size();

	// Get the tables of memnodes used in the send statement
	std::set<GraphNode*> mem1 = dg->getMemNodes1();
	std::set<GraphNode*> mem2 = dg->getMemNodes2();

	std::set<GraphNode*> mem2send1;
	std::set<GraphNode*> mem2send2;

	std::set<GraphNode*> visited;
	std::set<SegmentationTable*> tablesOfSend;
	SegmentationTable *rat;
	SegmentationTable *rat2;
	SegmentationTable *bigtable;

	// Iterates over every memory node |M| and over every send node |S|. Calls dfsVisit O(|M| * |S|) times
	errs()
			<< "Iterates over every memory node |M| and over every send node |S| ... ";
	// Loop for P1

	for (std::set<GraphNode*>::iterator send1it = sendP1.begin(), send1end =
			sendP1.end(); send1it != send1end; ++send1it) {

		for (std::set<GraphNode*>::iterator mem1it = mem1.begin(), mem1end =
				mem1.end(); mem1it != mem1end; ++mem1it) {
			// Get the reachable memnodes' tables

			if (dg->dfsVisit(*mem1it, *send1it, visited)) {

				// Get table of mem1it
				MemNode *m = dyn_cast<MemNode>(*mem1it);

				rat = ass.getTable(m->getAliasSetId());

				if(rat != NULL){

					tablesOfSend.insert(rat);

				}

			}

		}

		// Merge the tables building the big table of the send
		if (tablesOfSend.size() > 0) {

			rat = *tablesOfSend.begin();

			if (tablesOfSend.size() > 1) {

				std::set<SegmentationTable*>::iterator t1 =
						tablesOfSend.begin(), tend = tablesOfSend.end();
				for (++t1, tend = tablesOfSend.end(); t1 != tend; ++t1) {

					rat2 = *t1;

					bigtable = ass.UnionMerge(rat, rat2);
					ass.ContentMerge(bigtable, rat, rat2);

				}
			} else {

				bigtable = rat;

			}

			// Put the big table in the map sendP1Table
			sendP1Table[*send1it] = bigtable;

		}
		tablesOfSend.clear();
		visited.clear();

	}

	visited.clear();
	tablesOfSend.clear();

	// Loop for P2
	for (std::set<GraphNode*>::iterator send2it = sendP2.begin(), send2end =
			sendP2.end(); send2it != send2end; ++send2it) {

		for (std::set<GraphNode*>::iterator mem2it = mem2.begin(), mem2end =
				mem2.end(); mem2it != mem2end; ++mem2it) {
			// Get the reachable memnodes' tables

			if (dg->dfsVisit(*mem2it, *send2it, visited)) {

				// Get table of mem1it
				MemNode *m = dyn_cast<MemNode>(*mem2it);
				rat = ass.getTable(m->getAliasSetId());

				// Insert the table in the set
				tablesOfSend.insert(rat);

			}

		}

		// Merge the tables building the big table of the send
		if (tablesOfSend.size() > 0) {

			rat = *tablesOfSend.begin();

			if (tablesOfSend.size() > 1) {

				std::set<SegmentationTable*>::iterator t2 =
						tablesOfSend.begin(), tend = tablesOfSend.end();
				for (++t2, tend = tablesOfSend.end(); t2 != tend; ++t2) {
					rat2 = *t2;

					bigtable = ass.UnionMerge(rat, rat2);
					ass.ContentMerge(bigtable, rat, rat2);

				}
			} else {
				bigtable = rat;

			}

			// Put the big table in the map sendP2Table
			sendP2Table[*send2it] = bigtable;

		}
		tablesOfSend.clear();
		visited.clear();

	}

	visited.clear();
	errs() << "Done\n";

	std::map<GraphNode*, SegmentationTable*> allSendsToRecvTableMap1;
	std::set<SegmentationTable*> allSendsToRecvTables1;

	std::map<GraphNode*, SegmentationTable*> allSendsToRecvTableMap2;
	std::set<SegmentationTable*> allSendsToRecvTables2;

	// Check the Sends that reach directly each Recv
	errs() << "Check the Sends that reach directly each Recv ... ";
	// Iterates over every Send node |S| and over every Recv node |R|. Calls dfsVisit O(|S| * |R|) times

	// Loop P1
	for (std::set<GraphNode*>::iterator recv1it = recvP1.begin(), recv1end =
			recvP1.end(); recv1it != recv1end; ++recv1it) {

		for (std::set<GraphNode*>::iterator send2it = sendP2.begin(), send2end =
				sendP2.end(); send2it != send2end; ++send2it) {

			dg->dfsVisit(*send2it, *recv1it, visited);

			if (visited.count(*recv1it) > 0) {

				rat = sendP2Table[*send2it];
				allSendsToRecvTables1.insert(rat);
				NumNetEdges++;

			}

		}

		if (allSendsToRecvTables1.size() > 0) {

			rat = *allSendsToRecvTables1.begin();

			if (allSendsToRecvTables1.size() > 1) {

				std::set<SegmentationTable*>::iterator t1 =
						allSendsToRecvTables1.begin(), tend1 =
						allSendsToRecvTables1.end();

				for (++t1; t1 != tend1; ++t1) {
					rat2 = *t1;

					bigtable = ass.UnionMerge(rat, rat2);
					ass.ContentMerge(bigtable, rat, rat2);
					rat = bigtable;

				}

			} else {

				bigtable = rat;

			}

			// Set the propagate the bigtable to the Recv command
			allSendsToRecvTableMap1[(*recv1it)] = bigtable;

			allSendsToRecvTables1.clear();

		}
	}

	// Loop P2
	for (std::set<GraphNode*>::iterator recv2it = recvP2.begin(), recv2end =
			recvP2.end(); recv2it != recv2end; ++recv2it) {

		for (std::set<GraphNode*>::iterator send1it = sendP1.begin(), send1end =
				sendP1.end(); send1it != send1end; ++send1it) {

			dg->dfsVisit(*send1it, *recv2it, visited);

			if (visited.count(*recv2it) > 0) {

				rat = sendP1Table[*send1it];
				allSendsToRecvTables2.insert(rat);

				NumNetEdges++;

			}

		}

		if (allSendsToRecvTables2.size() > 0) {

			rat = *allSendsToRecvTables2.begin();

			if (allSendsToRecvTables2.size() > 1) {

				std::set<SegmentationTable*>::iterator t2 =
						allSendsToRecvTables2.begin(), tend2 =
						allSendsToRecvTables2.end();

				for (++t2; t2 != tend2; ++t2) {

					rat2 = *t2;

					bigtable = ass.UnionMerge(rat, rat2);
					ass.ContentMerge(bigtable, rat, rat2);
					rat = bigtable;

				}

			} else {

				bigtable = rat;

			}

			// Set the propagate the bigtable to the Recv command
			allSendsToRecvTableMap2[*recv2it] = bigtable;

			allSendsToRecvTables2.clear();

		}

	}

	errs() << "Done\n";

	//========================================================================================================================================================
	// For each recv, propagate the big table to the memnode the recv sets
	// Possible performance improvement here
	errs() << "Propagate the Recv tables to MemNodes ... ";
	for (std::set<GraphNode*>::iterator recv1it = recvP1.begin(), recv1end =
			recvP1.end(); recv1it != recv1end; ++recv1it) {

		for (std::set<GraphNode*>::iterator mem1it = mem1.begin(), mem1end =
				mem1.end(); mem1it != mem1end; ++mem1it) {

			if ((*recv1it)->getSuccessors().count(*mem1it)) {

				if (dg->dfsVisit(*recv1it, *mem1it, visited)
						&& visited.count(*mem1it) > 0) {

					int key = ((MemNode*) (*mem1it))->getAliasSetId();

					SegmentationTable *destMemSegTab = ass.getTable(key);
					SegmentationTable *recvMemSegTab = allSendsToRecvTableMap1[(*recv1it)];

					if(destMemSegTab != NULL && recvMemSegTab != NULL){

						SegmentationTable *auxTab = ass.UnionMerge(destMemSegTab, recvMemSegTab);
						ass.ContentMerge(auxTab, destMemSegTab, recvMemSegTab);

						memToTable[key] = auxTab;
						propagatedP1++;

					}



				}



			}
			visited.clear();

		}

	}

	for (std::set<GraphNode*>::iterator recv2it = recvP2.begin(), recv2end =
			recvP2.end(); recv2it != recv2end; ++recv2it) {

		for (std::set<GraphNode*>::iterator mem2it = mem2.begin(), mem2end =
				mem2.end(); mem2it != mem2end; ++mem2it) {

			if ((*recv2it)->getSuccessors().count(*mem2it)) {

				if (dg->dfsVisit(*recv2it, *mem2it, visited)
						&& visited.count(*mem2it)) {

					int key = ((MemNode*) (*mem2it))->getAliasSetId();

					SegmentationTable *destMemSegTab = ass.getTable(key);
					SegmentationTable *recvMemSegTab = allSendsToRecvTableMap1[(*recv2it)];

					if(destMemSegTab != NULL && recvMemSegTab != NULL){

						SegmentationTable *auxTab = ass.UnionMerge(destMemSegTab, recvMemSegTab);
						ass.ContentMerge(auxTab, destMemSegTab, recvMemSegTab);

						memToTable[key] = auxTab;
						propagatedP2++;

					}

				}

			}

			visited.clear();

		}

	}

	errs() << "Done\n";

	errs() << "=== P1 ===\n";
	errs() << "  Sends: " << sendP1.size() << "\n";
	errs() << "    Tables: " << sendP1Table.size() << "\n";
	errs() << "  Recvs: " << recvP1.size() << "\n";
	errs() << "  Tables received: " << allSendsToRecvTableMap1.size() << "\n";
	errs() << "  PropagatedTables: " << propagatedP1 << "\n";

	errs() << "=== P2 ===\n";
	errs() << "  Sends: " << sendP2.size() << "\n";
	errs() << "    Tables: " << sendP2Table.size() << "\n";
	errs() << "  Recvs: " << recvP2.size() << "\n";
	errs() << "  Tables received: " << allSendsToRecvTableMap2.size() << "\n";
	errs() << "  PropagatedTables: " << propagatedP2 << "\n";

	errs() << "AllPropagatedTables: " << memToTable.size() << "\n";

	// Calc of propagated segments
	for (std::map<int, SegmentationTable*>::iterator it = memToTable.begin(),
			itend = memToTable.end(); it != itend; ++it) {

		NumPropSegments += it->second->row_num;

		for (set<SegmentationTableRow*>::iterator r =
				(*it).second->rows.begin(), e = (*it).second->rows.end();
				r != e; ++r) {
			if (!(*r)->content.getLower().isMinSignedValue()
					|| !(*r)->content.getUpper().isMaxSignedValue()) {
				NumNonTrivPropSegments++;
			}
		}

	}

	return false;
}

Range ASSegPropagation::getRange(Value* Pointer) {

	Range range;
	range.setUnknown();

	AliasSets &as = getAnalysis<AliasSets>();
	int key = as.getValueSetKey(Pointer);

	llvm::DenseMap<int, std::set<llvm::Value*> > sets = as.getValueSets();
	std::set<llvm::Value*> set = sets[key];

	if (!memToTable.count(key)) {
		return range;
	} else {
		NumHitQueries++;
	}

	errs() << "===============================================\n";
	errs() << "Pointer name: " << Pointer->getName() << "\n";

	SegmentationTable* seg = memToTable[key];

	for(std::set<SegmentationTableRow*>::iterator it = seg->rows.begin(), itend = seg->rows.end(); it != itend; ++it){

		for(std::set<Value*>::iterator itval = (*it)->pointers.begin(), itvalend = (*it)->pointers.end(); itval != itvalend; ++itval){

			if((*itval)->getName() == Pointer->getName() ){
				errs() << "Pointer here. Row Range: "; (*it)->content.print(errs()); errs() << "\n";
				range = range.unionWith((*it)->content);
				errs() << "New range: "; range.print(errs()); errs() << "\n";
			}

		}
	}

	errs() << "Pointer new range: "; range.print(errs()); errs() << "\n";

	return range;

}

bool ASSegPropagation::isSend(GraphNode *node) {
	if ((node->getLabel().find("Write") != std::string::npos))
		return true;

	return false;

}

bool ASSegPropagation::isRecv(GraphNode *node) {

	if ((node->getLabel().find("Read") != std::string::npos))
		return true;

	return false;
}

bool ASSegPropagation::isNodeP1(GraphNode *node) {
	if (node->getName().find("P1") != std::string::npos)
		return true;

	return false;

}

char ASSegPropagation::ID = 0;
static RegisterPass<ASSegPropagation> X("as-set-propagation",
		"Propagation of segmentation of Alias Sets", false, false); /* namespace llvm */
