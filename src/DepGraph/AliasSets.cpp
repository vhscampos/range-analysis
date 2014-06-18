#define DEBUG_TYPE "AliasSets"
#include<AliasSets.h>

using namespace llvm;

bool AliasSets::runOnModule(Module &M) {
	PADriver &PD = getAnalysis<PADriver> ();
	PointerAnalysis* PA = PD.pointerAnalysis;



	std::map<int, std::set<int> > allPointsTo = PA->allPointsTo(); // sets of alias represented as ints
	/*
	 *   The sets represented by allPointsTo are not disjoint.
	 * 		Thus, we will process them and create disjoint sets that are useful for many other analyses.
	 */

	int count = 0;

	for (std::map<int, std::set<int> >::iterator i = allPointsTo.begin(), e =
			allPointsTo.end(); i != e; ++i) {

		bool found = false;
		int disjointSetIndex;

		//First, we discover the disjoint set that contains at least one of those values.
		//if none of the values are contained in any existing set, we create a new one

		if (!i->second.count(i->first)) {

			if (disjointSet.count(i->first)) {
				disjointSetIndex = disjointSet[i->first];
				found = true;
			}

		}

		if (!found){

			for (std::set<int>::iterator ii = i->second.begin(), ee = i->second.end(); ii != ee; ++ii) {
				if (disjointSet.count(*ii)) {
					disjointSetIndex = disjointSet[*ii];
					found = true;
					break;
				}
			}

			//Here we assign a new disjoint set index
			if (!found) disjointSetIndex = ++count;
		}

		//Here we create a new disjoint set
		if (!disjointSets.count(disjointSetIndex))  disjointSets[disjointSetIndex];


		std::set<int> &selectedDisjointSet = disjointSets[disjointSetIndex];


		//Finally, we insert the values in the selected disjoint set
		if (!i->second.count(i->first)) {

			if (!selectedDisjointSet.count(i->first)) {
				selectedDisjointSet.insert(i->first);
				disjointSet[i->first] = disjointSetIndex;
			}

		}

		for (std::set<int>::iterator ii = i->second.begin(), ee = i->second.end(); ii != ee; ++ii) {

			if (!selectedDisjointSet.count(*ii)) {
				selectedDisjointSet.insert(*ii);
				disjointSet[*ii] = disjointSetIndex;
			}

		}

	}


	//Now, we translate the disjoint sets to Value* disjoint sets

	//First we translate the keys
	for (llvm::DenseMap<int, int >::iterator i = disjointSet.begin(), e = disjointSet.end(); i != e; ++i) {

		if (PD.int2value.count(i->first)) {
			valueDisjointSet[PD.int2value[i->first]] = i->second;
		}

	}

	//Finally we translate the disjoint sets
	for (llvm::DenseMap<int, std::set<int> >::iterator i = disjointSets.begin(), e = disjointSets.end(); i != e; ++i) {

		std::set<Value*> translatedValues;

		for (std::set<int>::iterator ii = i->second.begin(), ee = i->second.end(); ii != ee; ++ii) {

			if (PD.int2value.count(*ii)) {
				translatedValues.insert(PD.int2value[*ii]);
			}
		}

		valueDisjointSets[i->first] = translatedValues;
	}

	//printSets();

	return false;
}


void AliasSets::printSets() {

	errs() << "------------------------------------------------------------\n"
		   << "							Alias sets:   				       \n"
		   << "------------------------------------------------------------\n\n";

	for (llvm::DenseMap<int, std::set<Value*> >::iterator i = valueDisjointSets.begin(), e = valueDisjointSets.end(); i != e; ++i) {

		errs() << "Set " << i->first << " :\n";

		std::set<Value*> translatedValues;

		for (std::set<Value*>::iterator ii = i->second.begin(), ee = i->second.end(); ii != ee; ++ii) {

			errs() << "	" << **ii << "\n";
		}

		errs() << "\n";

	}

}

void AliasSets::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.addRequired<PADriver> ();
	AU.setPreservesAll();
}

llvm::DenseMap<int, std::set<llvm::Value*> > AliasSets::getValueSets() {
	return valueDisjointSets;
}

llvm::DenseMap<int, std::set<int> > AliasSets::getMemSets() {
	return disjointSets;
}

int AliasSets::getValueSetKey(const Value* v) {

	if (valueDisjointSet.count(v)) return valueDisjointSet[v];

	//        assert(0 && "Value requested is not in any alias set!");
	return 0;

}

int AliasSets::getMapSetKey(int m) {

	if (disjointSet.count(m)) return disjointSet[m];
	//        assert(0 && "Memory positions requested is not in any alias set!");
	return 0;

}

char AliasSets::ID = 0;
static RegisterPass<AliasSets> X("alias-sets",
		"Get alias sets from pointer analysis pass", false, false);
