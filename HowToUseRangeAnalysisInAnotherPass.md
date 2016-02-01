Note: this tutorial is suited for LLVM 3.0. Some LLVM APIs and includes may have changed since then.

# Writing a Client Pass #

In order to use our range analysis, you can write a LLVM pass that calls it. There is [vast documentation](http://llvm.org/docs/WritingAnLLVMPass.html) about how to write LLVM passes in the web. The program below, which is self-contained, is an example of such a pass.

```
#include "llvm/Function.h"
#include "llvm/Pass.h"
#include "llvm/User.h"
#include "llvm/PassAnalysisSupport.h"
#include "llvm/Support/raw_ostream.h"
#include "../RangeAnalysis/RangeAnalysis.h"

using namespace llvm;

class ClientRA: public llvm::FunctionPass {
public:
	static char ID;
	ClientRA() : FunctionPass(ID){ }
	virtual ~ClientRA() { }
	virtual bool runOnFunction(Function &F){
		IntraProceduralRA<Cousot> &ra = getAnalysis<IntraProceduralRA<Cousot> >();

		errs() << "\nCousot Intra Procedural analysis (Values -> Ranges) of " << F.getName() << ":\n";
		for(Function::iterator bb = F.begin(), bbEnd = F.end(); bb != bbEnd; ++bb){
			for(BasicBlock::iterator I = bb->begin(), IEnd = bb->end(); I != IEnd; ++I){
				const Value *v = &(*I);
				Range r = ra.getRange(v);
				if(!r.isUnknown()){
					r.print(errs());
					I->dump();
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

char ClientRA::ID = 0;
static RegisterPass<ClientRA> X("client-ra", "A client that uses RangeAnalysis", false, false);
```

Our Range Analysis interface provides a method, `getRange`, that returns a `Range` object for any variable in the original code. This object of type `Range` contains the range information related to the variable. There are many versions of our range analysis pass, e.g., intra/inter procedural, with different narrowing operators, etc. In this example we are using the intra-procedural version using Cousot & Cousot's original narrowing operator.

# Steps to compile and run #

In order to use the example client, you need to give it a bitcode input file. Below we show how to do this. We shall be using, as an example, the following C program:
```
int foo() {
  int k = 0;
  while (k < 100) {
    int i = 0;
    int j = k;
    while (i < j) {
      i = i + 1;
      j = j - 1;
    }
    k = k + 1;
  }
  return k;
}

int main(int argc, char** argv) {
  printf("%d\n", foo());
}
```

  * First, we can translate a c file into a bitcode file using clang:
```
clang -c -emit-llvm test.c -o test.bc
```
  * Next thing: we must convert the bitcode file to e-SSA form. We do it using the `vssa` pass.
```
opt -instnamer -mem2reg -break-crit-edges test.bc -o test.bc
opt -load LLVM_SRC_PATH/Debug/lib/vSSA.so -vssa test.bc -o test.essa.bc
```
> Notice that we use a number of other passes too, to improve the quality of the code that we are producing:
    * `instnamer` just assigns strings to each variable. This will make the dot files that we produce to look nicer. We only use this pass for aesthetic reasons.
    * `mem2reg` maps variables allocated in the stack to virtual registers. Without this pass everything is mapped into memory, and then our range analysis will not be able to find any meaningful ranges to the variables.
    * `break-crit-edges` removes the critical edges from the control flow graph of the input program. This will increase the precision of our range analysis (just a tiny bit though), because the e-SSA transformation will be able to insert more sigma-functions into the code.

  * Now, we can run our example client. We can do this with the code below:
```
opt -load LLVM_SRC_PATH/Debug/lib/RangeAnalysis.so -load LLVM_SRC_PATH/Debug/lib/ClientRA.so -client-ra test.essa.bc
```

Once you run the code, and if nothing goes terribly wrong, you will have an output that should look - more or less - like this one below. Notice that we get a number of output lines for function `foo`, and zero for function `main`. Each output line is the range of a given variable, using LLVM's internal representation. It is important to use the `instnamer` pass, like we did before, to get some meaningful names for the variables. We did not get any line for `main` just because this function did not have any variable with a known range.
```
Cousot Intra Procedural analysis (Values -> Ranges) of foo:
[0, 100]  %k.0 = phi i32 [ 0, %bb ], [ %tmp9, %bb8 ]
[0, 99]  %vSSA_sigma = phi i32 [ %k.0, %bb1 ]
[-1, 99]  %j.0 = phi i32 [ %vSSA_sigma, %bb2 ], [ %tmp7, %bb5 ]
[0, 99]  %i.0 = phi i32 [ 0, %bb2 ], [ %tmp6, %bb5 ]
[0, 99]  %vSSA_sigma3 = phi i32 [ %j.0, %bb3 ]
[0, 98]  %vSSA_sigma2 = phi i32 [ %i.0, %bb3 ]
[1, 99]  %tmp6 = add nsw i32 %vSSA_sigma2, 1
[-1, 98]  %tmp7 = sub nsw i32 %vSSA_sigma3, 1
[1, 100]  %tmp9 = add nsw i32 %vSSA_sigma, 1
[100, 100]  %vSSA_sigma1 = phi i32 [ %k.0, %bb1 ]

Cousot Intra Procedural analysis (Values -> Ranges) of main:
```