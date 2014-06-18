/*
 * LoopInfoEx.cpp
 *
 *  Created on: Jan 11, 2014
 *      Author: raphael
 */

#include "LoopInfoEx.h"

namespace llvm {


void getNestedLoopsRec(Loop* L, std::set<Loop*> &NestedLoops){

	//Visits the nested loops of a given loop adding them to the set
	for(Loop::iterator it = L->begin(), iEnd = L->end(); it != iEnd; it++) {
		Loop* Tmp = *it;
		NestedLoops.insert(Tmp);
		getNestedLoopsRec(Tmp, NestedLoops);
	}

}


std::set<Loop*> LoopInfoEx::getNestedLoops(Loop* L){

	std::set<Loop*> result;
	getNestedLoopsRec(L, result);
	return result;

}


} /* namespace llvm */
