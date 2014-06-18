/*
 * ValueBranchMap.cpp
 *
 *  Created on: Mar 21, 2014
 *      Author: raphael
 */

#include "ValueBranchMap.h"


// ========================================================================== //
// ValueSwitchMap
// ========================================================================== //

ValueSwitchMap::ValueSwitchMap(const Value* V,
		SmallVector<std::pair<BasicInterval*, const BasicBlock*>, 4> BBsuccs) :
		V(V) {

	for (unsigned i = 0, e = BBsuccs.size(); i < e; ++i) {
		this->BBsuccs.push_back(BBsuccs[i]);
		BBids[this->BBsuccs[i].second] = i;
	}
}

ValueSwitchMap::~ValueSwitchMap() {
	clear();
}

void ValueSwitchMap::clear() {
	for (unsigned i = 0, e = BBsuccs.size(); i < e; ++i) {
		if (BBsuccs[i].first) {
			delete BBsuccs[i].first;
			BBsuccs[i].first = NULL;
		}
	}
}


// ========================================================================== //
// ValueBranchMap
// ========================================================================== //

ValueBranchMap::ValueBranchMap(const Value* V, const BasicBlock* BBTrue,
		const BasicBlock* BBFalse, BasicInterval* ItvT, BasicInterval* ItvF) :
		ValueSwitchMap(V){

	//False ==> index 0
	BBids[BBFalse] = 0;
	BBsuccs.push_back(std::pair<BasicInterval*, const BasicBlock*>(ItvF,BBFalse));

	//True ==> index 1
	BBids[BBTrue] = 1;
	BBsuccs.push_back(std::pair<BasicInterval*, const BasicBlock*>(ItvT,BBTrue));

}

ValueBranchMap::~ValueBranchMap() {

}
