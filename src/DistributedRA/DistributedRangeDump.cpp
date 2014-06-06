/*
 * DistributedRangeDump.cpp
 *
 *  Created on: Jun 5, 2014
 *      Author: raphael
 */

#include "DistributedRangeDump.h"

using namespace llvm;

bool DistributedRangeDump::runOnModule(Module &M){

	//Here we get the ranges of each load instruction and print it in an output file

	//TODO: Implement this method

	return false;
}

char DistributedRangeDump::ID = 0;
static RegisterPass<DistributedRangeDump> X("dist-range-dump",
"Dump of distributed value ranges", false, false);
