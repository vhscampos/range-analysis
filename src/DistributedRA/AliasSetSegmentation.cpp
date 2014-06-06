/*
 * AliasSetSegmentation.cpp
 *
 *  Created on: Jun 5, 2014
 *      Author: raphael
 */

#include "AliasSetSegmentation.h"

using namespace llvm;

bool AliasSetSegmentation::runOnModule(Module &M){

	//Here we compute the big tables per AliasSet

	//TODO: Implement this method

	return false;
}

RangedAliasTable* AliasSetSegmentation::getTable(int AliasSetID){

	//TODO: Implement this method

	return new RangedAliasTable;

}

char AliasSetSegmentation::ID = 0;
static RegisterPass<AliasSetSegmentation> X("as-segmentation",
"Segmentation of Alias Sets", false, false);
/* namespace llvm */
