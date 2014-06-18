/*
 * LoopInfoEx.h
 *
 *  Created on: Jan 11, 2014
 *      Author: raphael
 *
 *
 *  The class LoopInfoEx extends LoopInfo.
 *
 *  It overrides LoopInfo::Iterator. LoopInfo::Iterator only iterates over
 *  the top level loops, being useless to analyze nested loops.
 *
 *  LoopInfoEx::iterator iterates over the tree of nested loops in post order.
 *  Thus, the nested loop will always be visited before the containing loop.
 *
 */

#ifndef LOOPINFOEX_H_
#define LOOPINFOEX_H_

#include "llvm/Analysis/LoopInfo.h"
#include <vector>
#include <set>
#include <stack>

using namespace std;

namespace llvm {

class LoopInfoExIterator {

private:

	LoopInfo::iterator topLevelLoops;
	LoopInfo::iterator topLevelLoops_end;

	std::stack<std::pair<Loop::iterator, Loop::iterator> > innerLoops;

	bool isEnd;

public:

	void stackLoop(Loop* L){
		if (L->begin() != L->end()) {
			std::pair<Loop::iterator, Loop::iterator> tmp(L->begin(), L->end());
			innerLoops.push(tmp);

			Loop* l = *(L->begin());

			stackLoop(l);
		}
	}

	LoopInfoExIterator(LoopInfo::iterator initValue, LoopInfo::iterator endValue){

		topLevelLoops = initValue;
		topLevelLoops_end = endValue;

		if (topLevelLoops != topLevelLoops_end){
			Loop* l = *topLevelLoops;
			stackLoop(l);
		}

		isEnd = (topLevelLoops == topLevelLoops_end);

	}

	Loop* operator *() {
		if (innerLoops.size()) {
			std::pair<Loop::iterator, Loop::iterator> tmp = innerLoops.top();
			return *(tmp.first);
		} else {
			if (isEnd) assert(false && "Dereferencing out-of-bounds iterator!");
			return *topLevelLoops;
		}
	}

	bool operator==(LoopInfoExIterator RHS){

		if (this->isEnd && RHS.isEnd) return true;

		return (this->topLevelLoops == RHS.topLevelLoops && this->innerLoops == RHS.innerLoops);
	}

	bool operator!=(LoopInfoExIterator RHS){
		return ! (operator==(RHS));
	}

	void operator++(int Useless){
		if (innerLoops.size()) {

			std::pair<Loop::iterator, Loop::iterator> tmp = innerLoops.top();
			innerLoops.pop();

			Loop::iterator inner = tmp.first;
			inner++;

			if (inner != tmp.second) {
				std::pair<Loop::iterator, Loop::iterator> tmp2(inner, tmp.second);
				innerLoops.push(tmp2);

				//This loop may have nested loops. Push then into the stack
				Loop* l = *inner;
				stackLoop(l);
			}

		} else {
			topLevelLoops++;

			if (topLevelLoops != topLevelLoops_end) {
				Loop* l = *topLevelLoops;
				stackLoop(l);
			} else {
				isEnd = true;
			}
		}
	}

};


class LoopInfoEx: public LoopInfo {
public:
	typedef LoopInfoExIterator iterator;


	LoopInfoEx() : LoopInfo() {}
	virtual ~LoopInfoEx() {
		// TODO Auto-generated destructor stub
	}

	inline iterator begin(){
		iterator result(LoopInfo::begin(), LoopInfo::end());
		return result;
	}

	inline iterator end(){
		iterator result(LoopInfo::end(), LoopInfo::end());
		return result;
	}

	std::set<Loop*> getNestedLoops(Loop*);

};

} /* namespace llvm */

#endif /* LOOPINFOEX_H_ */
