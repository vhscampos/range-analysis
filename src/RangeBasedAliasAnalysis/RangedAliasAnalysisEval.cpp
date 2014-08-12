#include "llvm/Analysis/Passes.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Assembly/Writer.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Support/raw_ostream.h"
#include "RangedAliasTables.h"
using namespace llvm;

namespace llvm 
{

	class RAAEval : public FunctionPass 
	{
		unsigned NoAlias1a, MayAlias1a, PartialAlias1a, MustAlias1a, NTNoAlias, Total1, Total2;
		unsigned NoAlias1b, MayAlias1b, PartialAlias1b, MustAlias1b;
		unsigned NoAlias2a, MayAlias2a, PartialAlias2a, MustAlias2a;
		unsigned NoAlias2b, MayAlias2b, PartialAlias2b, MustAlias2b;
		unsigned ArrayNAa, StructNAa, ArrayNAb, StructNAb;
		
		RangedAliasTables* RAT;
		llvm::DenseMap<Value*, RangedPointer*> RangedPointerMap;
		llvm::DenseMap<Value*, std::set<RangedPointer*> > RangedPointerSets;
		llvm::DenseMap<Value*, RangedAliasTable*> RangedAliasTableMap;
		
		public:
		static char ID; // Class identification, replacement for typeinfo
		RAAEval() : FunctionPass(ID), RAT(0) 
    {
    	NoAlias1a = 0; 
    	MayAlias1a = 0;
    	PartialAlias1a = 0;
    	MustAlias1a = 0;
    	NoAlias1b = 0; 
    	MayAlias1b = 0;
    	PartialAlias1b = 0;
    	MustAlias1b = 0;
    	NoAlias2a = 0; 
    	MayAlias2a = 0;
    	PartialAlias2a = 0;
    	MustAlias2a = 0;
			NoAlias2b = 0; 
    	MayAlias2b = 0;
    	PartialAlias2b = 0;
    	MustAlias2b = 0;
    	
    	ArrayNAa = 0;
    	StructNAa = 0;
    	ArrayNAb = 0; 
    	StructNAb = 0;
    	
    	NTNoAlias = 0;
    	Total1 = 0;
    	Total2 = 0;
    }
    bool runOnFunction(Function &F);
    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired<AliasAnalysis>();
      AU.addRequired<RangedAliasTables>();
      AU.setPreservesAll();
    }
    virtual bool doFinalization(Module &M);
    
    AliasAnalysis::AliasResult alias(Value* p1, Value* p2);
	};

	char RAAEval::ID = 0;
	
	static RegisterPass<RAAEval> X("raa-eval", "Exhaustive Ranged Alias Analysis Precision Evaluator", false, false);
	
	static void PrintPercent(unsigned Num, unsigned Sum) 
	{
  	errs() << "(" << Num*100ULL/Sum << "."
         << ((Num*1000ULL/Sum) % 10) << "%)\n";
	}
	
	bool RAAEval::doFinalization(Module &M)
	{
  	errs() << "RESULTS:\n";
  	errs() << "1 TOTAL: " << Total1 << "\n\n";
  	errs() << "1a NoAlias: " << NoAlias1a << "\n";
  	errs() << "1a MayAlias: " << MayAlias1a << "\n";
  	errs() << "1a PartialAlias: " << PartialAlias1a << "\n";
  	errs() << "1a MustAlias: " << MustAlias1a << "\n\n";
  	errs() << "1b NoAlias: " << NoAlias1b << "\n";
  	errs() << "1b MayAlias: " << MayAlias1b << "\n";
  	errs() << "1b PartialAlias: " << PartialAlias1b << "\n";
  	errs() << "1b MustAlias: " << MustAlias1b << "\n\n";
  	errs() << "2 TOTAL: " << Total2 << "\n\n";
  	errs() << "2a NoAlias: " << NoAlias2a << "\n";
  	errs() << "2a MayAlias: " << MayAlias2a << "\n";
  	errs() << "2a PartialAlias: " << PartialAlias2a << "\n";
  	errs() << "2a MustAlias: " << MustAlias2a << "\n\n";
  	errs() << "2b NoAlias: " << NoAlias2b << "\n";
  	errs() << "2b MayAlias: " << MayAlias2b << "\n";
  	errs() << "2b PartialAlias: " << PartialAlias2b << "\n";
  	errs() << "2b MustAlias: " << MustAlias2b << "\n\n";
  	errs() << "Number of tables with NoAlias: " << NTNoAlias << "\n";
  	errs() << "ArrayNAa: " << ArrayNAa << "\n";
  	errs() << "StructNAa: " << StructNAa << "\n";
  	errs() << "ArrayNAb: " << ArrayNAb << "\n";
  	errs() << "StructNAb: " << StructNAb << "\n";
  }
  
  static inline bool isInterestingPointer(Value *V) {
  return V->getType()->isPointerTy()
      && !isa<ConstantPointerNull>(V);
	}
	
	bool RAAEval::runOnFunction(Function &F)
	{
		AliasAnalysis &AA = getAnalysis<AliasAnalysis>();
		RAT = &getAnalysis<RangedAliasTables>();
		RangedAliasTableMap = RAT->getRangedAliasTableMap();
		RangedPointerMap = RAT->getRangedPointerMap();
		RangedPointerSets = RAT->getRangedPointerSets();
		
		/*
		*
		* Pairs within tables
		*
		*/
		//iterating over tables
		for(llvm::DenseMap<Value*, RangedAliasTable*>::iterator i = RangedAliasTableMap.begin(), e = RangedAliasTableMap.end(); i != e; i++)
		{
		if(i->second->function == &F)
		{
			std::set<RangedPointer*> MRS = RangedPointerSets[i->first];
			RangedPointer* MRV[MRS.size()];
			int ind = 0;
			for(std::set<RangedPointer*>::iterator ii = MRS.begin(), ee = MRS.end(); ii != ee; ii++)
			{
				MRV[ind] = (*ii);
				ind++;
			}
			
			for(int j = 0; j < ind; j++)
				for(int k = j+1; k < ind; k++)
				{
					uint64_t I1Size = AliasAnalysis::UnknownSize;
					Type *I1ElTy =cast<PointerType>((MRV[j]->pointer)->getType())->getElementType();
		    	//if (I1ElTy->isSized()) I1Size = AA.getTypeStoreSize(I1ElTy);
					uint64_t I2Size = AliasAnalysis::UnknownSize;
					Type *I2ElTy =cast<PointerType>((MRV[k]->pointer)->getType())->getElementType();
		    	//if (I2ElTy->isSized()) I2Size = AA.getTypeStoreSize(I2ElTy);
					
					AliasAnalysis::AliasResult ranged = alias(MRV[j]->pointer, MRV[k]->pointer);
					AliasAnalysis::AliasResult other = AA.alias(MRV[j]->pointer, I1Size, MRV[k]->pointer, I2Size);
					Total1++;
					switch (other) 
					{
     				case AliasAnalysis::NoAlias:
        			NoAlias1a++;
							if(i->second->is_array == true)
								ArrayNAa++;
							else if(i->second->is_struct == true)
								StructNAa++;
							else if(i->second->is_vector == true)
								ArrayNAa++;
							else
								ArrayNAa++;
        			break;
      			case AliasAnalysis::MayAlias:
        			MayAlias1a++;
        			break;
      			case AliasAnalysis::PartialAlias:
        			PartialAlias1a++;
        			break;
      			case AliasAnalysis::MustAlias:
        			MustAlias1a++;
        			break;
        	}
        	if(ranged == AliasAnalysis::NoAlias || other == AliasAnalysis::NoAlias)
        	{
        		NoAlias1b++;
        		if(i->second->is_array == true)
							ArrayNAb++;
						else if(i->second->is_struct == true)
							StructNAb++;
						else if(i->second->is_vector == true)
							ArrayNAb++;
						else
							ArrayNAb++;
					}
					else
					{
						switch (other) 
						{
		   				case AliasAnalysis::MayAlias:
		      			MayAlias1b++;
		      			break;
		    			case AliasAnalysis::PartialAlias:
		      			PartialAlias1b++;
		      			break;
		    			case AliasAnalysis::MustAlias:
		      			MustAlias1b++;
		      			break;
		      	}
					}
				
				
				}
		}
		}
		
		
		/*
		*
		* Pairs within function
		*
		*/
		SetVector<Value *> Pointers;
		SetVector<CallSite> CallSites;
		SetVector<Value *> Loads;
		SetVector<Value *> Stores;

		for (Function::arg_iterator I = F.arg_begin(), E = F.arg_end(); I != E; ++I)
		  if (I->getType()->isPointerTy())    // Add all pointer arguments.
		    Pointers.insert(I);

		for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
		  if (I->getType()->isPointerTy()) // Add all pointer instructions.
		    Pointers.insert(&*I);
		  /*if (EvalTBAA && isa<LoadInst>(&*I))
		    Loads.insert(&*I);
		  if (EvalTBAA && isa<StoreInst>(&*I))
		    Stores.insert(&*I);*/
		  Instruction &Inst = *I;
		  if (CallSite CS = cast<Value>(&Inst)) {
		    Value *Callee = CS.getCalledValue();
		    // Skip actual functions for direct function calls.
		    if (!isa<Function>(Callee) && isInterestingPointer(Callee))
		      Pointers.insert(Callee);
		    // Consider formals.
		    for (CallSite::arg_iterator AI = CS.arg_begin(), AE = CS.arg_end();
		         AI != AE; ++AI)
		      if (isInterestingPointer(*AI))
		        Pointers.insert(*AI);
		    CallSites.insert(CS);
		  } else {
		    // Consider all operands.
		    for (Instruction::op_iterator OI = Inst.op_begin(), OE = Inst.op_end();
		         OI != OE; ++OI)
		      if (isInterestingPointer(*OI))
		        Pointers.insert(*OI);
		  }
		}

		// iterate over the worklist, and run the full (n^2)/2 disambiguations
		for (SetVector<Value *>::iterator I1 = Pointers.begin(), E = Pointers.end();
		     I1 != E; ++I1) {
		  uint64_t I1Size = AliasAnalysis::UnknownSize;
		  Type *I1ElTy = cast<PointerType>((*I1)->getType())->getElementType();
		  //if (I1ElTy->isSized()) I1Size = AA.getTypeStoreSize(I1ElTy);

		  for (SetVector<Value *>::iterator I2 = Pointers.begin(); I2 != I1; ++I2) {
		    
		    uint64_t I2Size = AliasAnalysis::UnknownSize;
		    Type *I2ElTy =cast<PointerType>((*I2)->getType())->getElementType();
		    //if (I2ElTy->isSized()) I2Size = AA.getTypeStoreSize(I2ElTy);

				AliasAnalysis::AliasResult ranged = alias(*I1, *I2);
				AliasAnalysis::AliasResult other = AA.alias(*I1, I1Size, *I2, I2Size);
				Total2++;
		    switch (other) 
				{
   				case AliasAnalysis::NoAlias:
       			NoAlias2a++;
       			break;
     			case AliasAnalysis::MayAlias:
       			MayAlias2a++;
       			break;
     			case AliasAnalysis::PartialAlias:
       			PartialAlias2a++;
       			break;
     			case AliasAnalysis::MustAlias:
       			MustAlias2a++;
       			break;
       	}
       	if(ranged == AliasAnalysis::NoAlias || other == AliasAnalysis::NoAlias)
       	{
       		NoAlias2b++;
				}
				else
				{
					switch (other) 
					{
	   				case AliasAnalysis::MayAlias:
	      			MayAlias2b++;
	      			break;
	    			case AliasAnalysis::PartialAlias:
	      			PartialAlias2b++;
	      			break;
	    			case AliasAnalysis::MustAlias:
	      			MustAlias2b++;
	      			break;
	      	}
				}
	  
		  }
		}
		
		return false;
	}

/*
*
* ranged-aa
*
*/
AliasAnalysis::AliasResult 
RAAEval::alias(Value* p1, Value* p2)
{
	RangedPointer* m1 = RangedPointerMap[p1];
	RangedPointer* m2 = RangedPointerMap[p2];
	RangedAliasTable *t1, *t2;
	if (m1 != NULL) t1 = RangedAliasTableMap[m1->father];
	else t1 = NULL;
	if (m2 != NULL) t2 = RangedAliasTableMap[m2->father];
	else t2 = NULL;

	if( (t1 == t2) && (t1 != NULL) )
	{
		bool mark = false;
		if( m1->offset.getLower().slt(m2->offset.getLower()) && m1->offset.getUpper().slt(m2->offset.getLower()) )
			mark = true;
		else if( m2->offset.getLower().slt(m1->offset.getLower()) && m2->offset.getUpper().slt(m1->offset.getLower()) )
			mark = true;
		
		if(mark == true)
		{ 
			if(t1->has_no_alias == false)
			{
				t1->has_no_alias = true;
				NTNoAlias++;
			}
			/*if(t1->is_array == true)
				Array1++;
			else if(t1->is_struct == true)
				Struct1++;
			else if(t1->is_vector == true)
				Vector1++;
			else
				Array1++;
			
			SameTables1++;
			NoAlias1++;*/
			return AliasAnalysis::NoAlias;
		}
		else
			return AliasAnalysis::MayAlias;
	}
	else if ( (t1 != NULL) && (t2 != NULL) )
	{
		if(t1->aloc_base && t2->aloc_base)
		{
			//DifTables1++;
			//NoAlias1++;
			return AliasAnalysis::NoAlias;
		}
		else
		{
			//MayAlias1++;
			return AliasAnalysis::MayAlias;
		}
	}
	else
	{
		//MayAlias1++;
		return AliasAnalysis::MayAlias;
	}
}

}

