#include "llvm/Function.h"
#include "llvm/Pass.h"
#include "llvm/User.h"
#include "llvm/PassAnalysisSupport.h"
#include "llvm/Support/raw_ostream.h"
#include "../RangeAnalysis/RangeAnalysis.h"
#include "RAInstrumentation.h"

using namespace llvm;

class RAPrinter: public llvm::ModulePass {
public:
        static char ID;
        RAPrinter() : ModulePass(ID){ }
        virtual ~RAPrinter() { }
        virtual bool runOnModule(Module &M){
                IntraProceduralRA<Cousot> &ra = getAnalysis<IntraProceduralRA<Cousot> >();

                std::string Filename = "RAEstimatedValues.txt";

                std::string ErrorInfo;
                raw_fd_ostream File(Filename.c_str(), ErrorInfo);

                if (!ErrorInfo.empty()){
                  errs() << "Error opening file RAEstimatedValues.txt for writing! \n";
                  return false;
                }


            	// Iterate through functions
            	for (Module::iterator Fit = M.begin(), Fend = M.end(); Fit != Fend; ++Fit) {

            		// If the function is empty, there is nothing to do...
            		if (Fit->begin() == Fit->end())
            			continue;

            		// Iterate through basic blocks
            		for (Function::iterator BBit = Fit->begin(), BBend = Fit->end(); BBit != BBend; ++BBit) {

            			// Iterate through instructions
            			for (BasicBlock::iterator Iit = BBit->begin(); Iit != BBit->end(); ++Iit) {

            				llvm::Instruction* I = cast<Instruction>(Iit);
            				if ( RAInstrumentation::isValidInst(I) ){

                                const Value *v = &(*Iit);
                                Range r = ra.getRange(v);

                                //Prints the variable and the range to the output file
                                File << M.getModuleIdentifier()
                                     << "." << Fit->getName().str()
                                     << "." << cast<Value>(Iit)->getName()
                                     << " " r.getLower()
                                     << " " r.getUpper() << "\n";

            				}
            			}
            		}
            	}



                return false;
        }

        virtual void getAnalysisUsage(AnalysisUsage &AU) const {
                AU.setPreservesAll();
                AU.addRequired<IntraProceduralRA<Cousot> >();
        }
};

char RAPrinter::ID = 0;
static RegisterPass<RAPrinter> X("ra-printer", "RangeAnalysis printer.", false, false);
