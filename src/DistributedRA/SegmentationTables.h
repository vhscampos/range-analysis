#ifndef __SEGMENTATION_TABLES_H__
#define __SEGMENTATION_TABLES_H__

#include "llvm/Pass.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/APInt.h"
#include <set>
#include <list>
#include <vector>
#include <algorithm>
#include "../RangeAnalysis/RangeAnalysis.h"

using namespace std;

namespace llvm
{

	//Holds a memory range for a determined Value
	struct RangedPointer 
	{
		Value* pointer;
		Value* father;
		Range offset;
		Function* func;
		
		RangedPointer(Value* Pointer, APInt Lower, APInt Higher, Value* Father, Function* Func)
		{
			pointer = Pointer;
			offset.setLower(Lower);
			offset.setUpper(Higher);
			father = Father;
			func = Func;
		}
		//Finds the RangedPointer of element in a RangedPointer set
		static RangedPointer* 
			FindByValue
				(Value* element, std::set<RangedPointer*> MRSet);
	};

	//Definitions for our tables
	struct SegmentationTableRow
	{
		Range offset;
		Range content;
		set<Value*> pointers;
	};

	struct SegmentationTable
	{
		set<SegmentationTableRow*> rows;
		//RangedPointer present in this table
		std::set<RangedPointer*> ranged_pointers;
		//A map that tells which rows each pointer is present
		llvm::DenseMap<Value*, std::set<SegmentationTableRow*> > pointer_to_rows;
		
		//number of non empty rows
		int row_num;
		//Associations
			//Table's base pointer
		Value* base;
			//Is an allocation table?
		bool aloc_base;
			//Tables function
		Function* function;
			//It's function is recursive?
		bool recursive_func;
			//Pointer to it's alias set
		std::set<Value*>* AliasSet;
			//Does It's pointer comprise of a full alias set
		bool full_alias_set;
			//During Alias Analysis, did this table issued a NoAlias?
		bool has_no_alias;
			//This table came from what?
		bool is_struct;
		bool is_array;
		bool is_vector;
		bool is_parameter;
		bool is_load;
		bool is_global;
		
		void PrintPointers();
		void PrintContent();
	
	};

	class SegmentationTables: public ModulePass
	{
		private:
			//Holds a Primitive Layout for a determined Type
			struct PrimitiveLayout
			{
				Type * type;
				std::vector<int> layout;
				PrimitiveLayout(Type* ty, std::vector<int> lay)
				{
					type = ty;
					layout = lay;
				}
			};
			struct NumPrimitive
			{
				Type * type;
				int num;
				NumPrimitive(Type* ty, int n)
				{
					type = ty;
					num = n;
				}
			};
			std::vector<PrimitiveLayout*> PrimitiveLayouts;
			std::vector<NumPrimitive*> NumPrimitives;
			std::vector<int> getPrimitiveLayout(Type* type);
			int getNumPrimitives(Type* type);
			llvm::Type* getTypeInside(Type* type, int i);
			int getSumBehind(std::vector<int> v, int i);
		
			//Persistent maps
			llvm::DenseMap<Value*, RangedPointer*> RangedPointerMap;
			std::set<Value*> NotUsedPointers;
			llvm::DenseMap<Value*, std::set<RangedPointer*> > RangedPointerSets;
			llvm::DenseMap<Value*, SegmentationTable*> SegmentationTableMap;
			
			//Methods for debugging
			void printRangeAnalysis(InterProceduralRA<Cousot> *ra, Module *M);
			void printRangedPointerMap();
			void printNotUsedPointers();
			void printPrimitiveLayouts(std::vector<PrimitiveLayout*> PrimitiveLayouts);
			void printSegmentationTableMap();
			
		public:
			//LLVM framework methods and atributes
			static char ID;
			SegmentationTables() : ModulePass(ID){}
			bool runOnModule(Module &M);
			void getAnalysisUsage(AnalysisUsage &AU) const;
			
			//methods that return the persistent maps
			llvm::DenseMap<Value*, RangedPointer*> getRangedPointerMap();
			std::set<Value*> getNotUsedPointers();
			llvm::DenseMap<Value*, std::set<RangedPointer*> > getRangedPointerSets();
			llvm::DenseMap<Value*, SegmentationTable*> getSegmentationTableMap();
	};
}

#endif
