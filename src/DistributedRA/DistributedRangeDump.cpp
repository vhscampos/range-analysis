/*
 * DistributedRangeDump.cpp
 *
 *  Created on: Jun 5, 2014
 *      Author: raphael
 */

#include "DistributedRangeDump.h"

using namespace llvm;

cl::opt<std::string> DumpFilename("dump-filename",
		                          cl::desc("Specify dump filename"),
		                          cl::value_desc("filename"),
		                          cl::init("stderr"),
		                          cl::NotHidden);

std::string llvm::DistributedRangeDump::getOriginalFunctionName(Function* F) {

	//FIXME: remove the prefix of the function names.

	return F->getName();
}

bool DistributedRangeDump::runOnModule(Module &M){

	raw_ostream *outputStream = NULL;
	raw_fd_ostream* File = NULL;

	//Define the output stream
	if(DumpFilename.compare("stderr") == 0) outputStream = &errs();
	else {
		std::string ErrorInfo;

		File = new raw_fd_ostream(DumpFilename.c_str(), ErrorInfo);

        if (!ErrorInfo.empty()) {
                errs() << "Error opening file " << DumpFilename
                       << " for writing! Error Info: " << ErrorInfo << " \n";
                return false;
        }

        outputStream = File;
	}

	//ASSegPropagation& ASP = getAnalysis<ASSegPropagation>();

	//Iterate over functions
	for(Module::iterator Fit = M.begin(), Fend = M.end(); Fit != Fend; Fit++){

		std::string FunctionName;
		if (Fit->begin() != Fit->end()) {
			FunctionName = getOriginalFunctionName(Fit);
		}

		//Iterate over basic blocks
		for (Function::iterator BBit = Fit->begin(), BBend = Fit->end(); BBit != BBend; BBit++){

			//Iterate over instructions
			for(BasicBlock::iterator Iit = BBit->begin(), Iend = BBit->end(); Iit != Iend; Iit++){

				Instruction* I = Iit;

				if(LoadInst* LI = dyn_cast<LoadInst>(I)){

					Range r(Min,Max); // = ASP.getRange(LI->getPointerOperand());

					(*outputStream) << FunctionName  << "|"
									<< LI->getName() << "|"
									<< r.getLower()  << "|"
									<< r.getUpper()  << "\n";

				}
			}
		}
	}

	if (File) {
		File->close();
		delete File;
	}

	return false;
}

char DistributedRangeDump::ID = 0;
static RegisterPass<DistributedRangeDump> X("dump-dist-ranges",
"Dump of distributed value ranges", false, false);
