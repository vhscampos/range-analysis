/*
 * ASSegPropagation.cpp
 *
 *  Created on: Jun 5, 2014
 *      Author: raphael
 */

#include "ASSegPropagation.h"

using namespace llvm;

bool ASSegPropagation::runOnModule(Module &M){

	//Here we compute the propagation of the big tables

	//TODO: Implement this method

	return false;
}

Range ASSegPropagation::getRange(Value* Pointer){

	//TODO: Implement this method

	return Range();

}

char ASSegPropagation::ID = 0;
static RegisterPass<ASSegPropagation> X("as-set-propagation",
"Propagation of segmentation of Alias Sets", false, false); /* namespace llvm */
