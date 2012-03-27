#include "llvm/Function.h"
#include "llvm/Pass.h"
#include "llvm/User.h"
#include "llvm/PassAnalysisSupport.h"
#include "llvm/Support/raw_ostream.h"
#include "../RangeAnalysis/RangeAnalysis.h"
#include "../RAInstrumentation/RAInstrumentation.h"

using namespace llvm;

bool isValidInst(Instruction *I)
{
	// Only i32 instructions are valid
	// Exceptions: invoke instructions
	bool conditions = !I->getType()->isIntegerTy(BIT_WIDTH) || isa<InvokeInst>(I);

	return !conditions;
}
namespace {
	class RAPrinterInterProceduralCousot: public llvm::ModulePass {
	public:
			static char ID;
			RAPrinterInterProceduralCousot() : ModulePass(ID){ }
			virtual ~RAPrinterInterProceduralCousot() { }
			virtual bool runOnModule(Module &M){

					InterProceduralRA<Cousot> &ra = getAnalysis<InterProceduralRA<Cousot> >();

				    int pos = M.getModuleIdentifier().rfind("/");
					std::string mIdentifier = pos > 0 ? M.getModuleIdentifier().substr(pos + 1) : M.getModuleIdentifier();

					std::string Filename = "/tmp/RAEstimatedValues." + mIdentifier + ".txt";

					std::string ErrorInfo;
					raw_fd_ostream File(Filename.c_str(), ErrorInfo);

					if (!ErrorInfo.empty()){
					  errs() << "Error opening file /tmp/RAEstimatedValues." << mIdentifier << ".txt for writing! Error Info: " << ErrorInfo  << " \n";
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
								if ( isValidInst(I) ){

									const Value *v = &(*Iit);
									Range r = ra.getRange(v);

									//Prints the variable and the range to the output file
									File << mIdentifier
										 << "." << Fit->getName().str()
										 << "." << cast<Value>(Iit)->getName()
										 << " " << r.getLower()
										 << " " << r.getUpper() << "\n";

								}
							}
						}
					}



					return false;
			}

			virtual void getAnalysisUsage(AnalysisUsage &AU) const {
					AU.setPreservesAll();
					AU.addRequired<InterProceduralRA<Cousot> >();
			}

	};
}

char RAPrinterInterProceduralCousot::ID = 0;
static RegisterPass<RAPrinterInterProceduralCousot> Y("ra-printer-inter-cousot", "RangeAnalysis printer (Interprocedural - Cousot).", false, false);
