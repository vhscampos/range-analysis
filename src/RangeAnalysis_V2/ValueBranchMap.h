/*
 * ValueBranchMap.h
 *
 *  Created on: Mar 21, 2014
 *      Author: raphael
 */

#ifndef VALUEBRANCHMAP_H_
#define VALUEBRANCHMAP_H_

#include "BasicInterval.h"
#include "SymbolicInterval.h"

using namespace llvm;

/*
 * These classes are used to store the information that we learn from the
 * conditional branches.
 * I decided to write them in order to make the source code easier to understand.
 *
 * Instead, we could attach the information directly in the constraint graph,
 * what would be more efficient, but less maintainable.
 */

class ValueSwitchMap {
protected:
	const Value* V;
	SmallVector<std::pair<BasicInterval*, const BasicBlock*>, 4 > BBsuccs;
	std::map<const BasicBlock*, int> BBids;
	ValueSwitchMap(const Value* V): V(V){};

public:
	ValueSwitchMap(const Value* V,
		SmallVector<std::pair<BasicInterval*, const BasicBlock*>, 4 > BBsuccs);

	~ValueSwitchMap();
	/// Get the "false side" of the branch
	const BasicBlock *getBB(unsigned idx) const {
		return BBsuccs[idx].second;
	}
	/// Get the interval associated to the true side of the branch
	BasicInterval *getItv(unsigned idx) const {
		return BBsuccs[idx].first;
	}
	// Get how many cases this switch has
	unsigned getNumOfCases() const {
		return BBsuccs.size();
	}
	/// Get the value associated to the branch.
	const Value *getV() const {
		return V;
	}
	/// Change the interval associated to the true side of the branch
	void setItv(unsigned idx, BasicInterval *Itv) {
		this->BBsuccs[idx].first = Itv;
	}

	int getBBid(const BasicBlock* BB){
		if(!BBids.count(BB)){
			return -1;
		} else {
			return BBids[BB];
		}
	}

	/// Clear memory allocated
	void clear();
};


class ValueBranchMap: public ValueSwitchMap {
public:
	ValueBranchMap(const Value* V,
		const BasicBlock* BBTrue,
		const BasicBlock* BBFalse,
		BasicInterval* ItvT,
		BasicInterval* ItvF);
	~ValueBranchMap();

	/// Get the "false side" of the branch
	const BasicBlock *getBBFalse() const {
		return getBB(0);
	}
	/// Get the "true side" of the branch
	const BasicBlock *getBBTrue() const {
		return getBB(1);
	}

	/// Get the interval associated to the false side of the branch
	BasicInterval *getItvF() const {
		return getItv(0);
	}
	/// Get the interval associated to the true side of the branch
	BasicInterval *getItvT() const {
		return getItv(1);
	}


	/// Change the interval associated to the false side of the branch
	void setItvF(BasicInterval *Itv) {
		setItv(0, Itv);
	}
	/// Change the interval associated to the true side of the branch
	void setItvT(BasicInterval *Itv) {
		setItv(1, Itv);
	}
};


#endif /* VALUEBRANCHMAP_H_ */
