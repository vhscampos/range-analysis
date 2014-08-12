#define DEBUG_TYPE "ranged-aa"
#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "RangedAliasTables.h"
#include "llvm/Pass.h"
#include "llvm/ADT/DenseMap.h"

using namespace llvm;

STATISTIC(NTNoAlias, "Number of table with disjoint pointers");
STATISTIC(DifTablesNA, "Number of NoAlias from different aloc tables");
STATISTIC(StructNA, "Number of NoAlias from struct");
STATISTIC(ArrayNA, "Number of NoAlias from array");
STATISTIC(VectorNA, "Number of NoAlias from vector");
STATISTIC(SameTablesNA, "Number of NoAlias from same tables");
STATISTIC(NA, "Number of NoAlias issued");

namespace llvm {
	/// RangedAliasAnalysis - This is a simple alias analysis
  /// implementation that uses RagedAliasTables to answer queries.
  	
	class RangedAliasAnalysis : public FunctionPass, public AliasAnalysis{

		RangedAliasTables* RAT;
	  llvm::DenseMap<Value*, RangedAliasTable*> RangedAliasTableMap;
	  llvm::DenseMap<Value*, RangedPointer*> RangedPointerMap;

	  public:
    static char ID; // Class identification, replacement for typeinfo
    RangedAliasAnalysis() : FunctionPass(ID), RAT(0) 
    {
    	NTNoAlias = 0; 
    	DifTablesNA = 0;
    	StructNA = 0;
    	ArrayNA = 0;
    	VectorNA = 0;
    	SameTablesNA = 0;
    	NA = 0;
    }

		/// getAdjustedAnalysisPointer - This method is used when a pass implements
    /// an analysis interface through multiple inheritance.  If needed, it
    /// should override this to adjust the this pointer as needed for the
    /// specified pass info.
    virtual void *getAdjustedAnalysisPointer(AnalysisID PI) {
      if (PI == &AliasAnalysis::ID)
        return (AliasAnalysis*)this;
      return this;
    }

    private:
    virtual void getAnalysisUsage(AnalysisUsage &AU) const;
    virtual bool runOnFunction(Function &F);
    virtual AliasResult alias(const Location &LocA, const Location &LocB);
    RangedAliasTable* getTable(Value* p);
  };
}  // End of anonymous namespace

// Register this pass...
char RangedAliasAnalysis::ID = 0;
static RegisterPass<RangedAliasAnalysis> X("ranged-aa",
"RangedAliasTables-based Alias Analysis", false, false);
static RegisterAnalysisGroup<AliasAnalysis> E(X);

void
RangedAliasAnalysis::getAnalysisUsage(AnalysisUsage &AU) const {
  AliasAnalysis::getAnalysisUsage(AU);
  AU.addRequired<RangedAliasTables>();
  AU.setPreservesAll();
}

bool
RangedAliasAnalysis::runOnFunction(Function &F) {
  InitializeAliasAnalysis(this);
  RAT = &getAnalysis<RangedAliasTables>();
  RangedAliasTableMap = RAT->getRangedAliasTableMap();
  RangedPointerMap = RAT->getRangedPointerMap();
  return false;
}

RangedAliasAnalysis::AliasResult 
RangedAliasAnalysis::alias(const Location &LocA, const Location &LocB)
{
	Value *p1, *p2; 
	p1 = (Value*)LocA.Ptr;
	p2 = (Value*)LocB.Ptr;
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
			if(t1->is_array == true)
				ArrayNA++;
			else if(t1->is_struct == true)
				StructNA++;
			else if(t1->is_vector == true)
				VectorNA++;
			else
				ArrayNA++;
			
			SameTablesNA++;
			NA++;
			return NoAlias;
		}
		else
			return AliasAnalysis::alias(LocA, LocB);
	}
	else if ( (t1 != NULL) && (t2 != NULL) )
	{
		if(t1->aloc_base && t2->aloc_base)
		{
			DifTablesNA++;
			NA++;
			return NoAlias;
		}
		else
			//return NoAlias;
			return AliasAnalysis::alias(LocA, LocB);
	}
	else
		return AliasAnalysis::alias(LocA, LocB);
}

RangedAliasTable* 
RangedAliasAnalysis::getTable
(Value* p)
{
	for(llvm::DenseMap<Value*, RangedAliasTable*>::iterator
	i = RangedAliasTableMap.begin(), e = RangedAliasTableMap.end(); i != e; i++)
	{
		RangedAliasTable* table = i->second;
		for(set<RangedAliasTableRow*>::iterator ii = table->rows.begin(),
		ee = table->rows.end(); ii != ee; ii++)
		{
			if((*ii)->pointers.find(p) != (*ii)->pointers.end())
				return table;
		}
	}
	return NULL;
}
