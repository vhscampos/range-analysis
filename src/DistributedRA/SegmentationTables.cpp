#define DEBUG_TYPE "segmentation-tables"
#include "SegmentationTables.h"

using namespace llvm;

/*
* Statistics macros and global variables
*/

// These macros are used to get stats regarding the precision of our analysis.
STATISTIC(NTables, "Number of relevant tables generated");
STATISTIC(NTableAvgSize, "Average size of relevant tables");
STATISTIC(NTableMaxSize, "Size of the biggest table");
STATISTIC(PointersUsed, "Percentage of pointers contemplated");
STATISTIC(NATables, "Number of tables generated");
STATISTIC(NStruct, "Number of struct tables generated");
STATISTIC(NArray, "Number of array tables generated");
STATISTIC(NVector, "Number of vector tables generated");
STATISTIC(NParameter, "Number of parameter tables generated");
STATISTIC(NLoad, "Number of load tables generated");
STATISTIC(NGlobal, "Number of global tables generated");
//Global Variables Declarations
extern APInt Min;
extern APInt Max;
extern APInt Zero;

/*
* Safe APInt adition and multiplication and comparison
*/

bool 
compAPInt 
(APInt i,APInt j) 
{ 
	return (i.slt(j)); 
}

APInt //returns 
safe_addition //name
(APInt x, APInt y) //parameters
{
	if(x == Min or y == Min)
		return Min;
	else if(x == Max or y == Max)
		return Max;
		
	if(x.isNonNegative() and y.sgt(Max - x))
	{
		return Max;
	}
	else if (x.isNegative() and y.slt(Min - x))
	{
		return Min;
	}
	else
	{
		return x + y;
	}
}

APInt //returns 
safe_multiplication //name
(APInt x, APInt y) //parameters
{
	if(x == Zero or y == Zero)
		return Zero;
	
	if(x.isNonNegative())//x is positive
	{
		if(y.isNonNegative())//y is positive
		{
			if(x == Max or y == Max)
				return Max;
			else if(x.sgt(Max.sdiv(y)))
				return Max;
			else
				return x * y;
		}
		else //y is negative
		{
			APInt aux;
			if(y == -1)
				aux = Max;
			else
				aux = Min.sdiv(y);
			
			if(x == Max or y == Min)
				return Min;
			else if(x.sgt(aux))
				return Min;
			else
				return x * y;
		}
	}
	else //x is negative
	{
		if(y.isNonNegative())//y is positive
		{
			if(x == Min or y == Max)
				return Min;
			else if(x.slt(Min.sdiv(y)))
				return Min;
			else
				return x * y;
		}
		else //y is negative
		{
			APInt aux;
			if(y == -1)
				aux = Min;
			else
				aux = Max.sdiv(y);
		
			if(x == Min or y == Min)
				return Max;
			else if(x.slt(Max.sdiv(y)))
				return Max;
			else
				return x * y;
		}
	}
	
}

/*
*	BitWidth functions
*/

unsigned 
getMaxBitWidth
(const Function& F) 
{
	unsigned int InstBitSize = 0, opBitSize = 0, max = 0;

	// Obtains the maximum bit width of the instructions of the function.
	for (const_inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
		InstBitSize = I->getType()->getPrimitiveSizeInBits();
		if (I->getType()->isIntegerTy() && InstBitSize > max) {
			max = InstBitSize;
		}

		// Obtains the maximum bit width of the operands of the instruction.
		User::const_op_iterator bgn = I->op_begin(), end = I->op_end();
		for (; bgn != end; ++bgn) {
			opBitSize = (*bgn)->getType()->getPrimitiveSizeInBits();
			if ((*bgn)->getType()->isIntegerTy() && opBitSize > max) {
				max = opBitSize;
			}
		}
	}

	// Bitwidth equal to 0 is not valid, so we increment to 1
	if (max == 0) ++max;
	
	return max;
}

unsigned 
getMaxBitWidth
(Module &M) 
{
	unsigned max = 0;
	// Search through the functions for the max int bitwidth
	for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I) {
		if (!I->isDeclaration()) {
			unsigned bitwidth = RangeAnalysis::getMaxBitWidth(*I);

			if (bitwidth > max)
				max = bitwidth;
		}
	}
	return max;
}

/*
* RangedPointer Support functions
*/

//Finds the RangedPointer of element in a RangedPointer set
RangedPointer* //Returns
RangedPointer::FindByValue //Name
(Value* element, std::set<RangedPointer*> MRSet) //Parameters
{
	for(std::set<RangedPointer*>::iterator i = MRSet.begin(), e = MRSet.end(); 
	i != e;	i++)
		if(element == (*i)->pointer)
			return *i;
	
	return NULL;
}

/*
* Calculate Primitive Layouts and Number of elements.
*/

//Returns the sum of previous elements of vector
int 
SegmentationTables::getSumBehind
(std::vector<int> v, int i)
{
	int s = 0;
	if(i > v.size())
		i = v.size();
	for(int j = i-1; j >= 0; j--)
		s += v[j];
	return s;
}

//Returns the type of the ith element inside type
Type* 
SegmentationTables::getTypeInside
(Type* type, int i)
{
	if(type->isPointerTy())
		return type->getPointerElementType();
	else if(type->isArrayTy())
		return type->getArrayElementType();
	else if(type->isStructTy())
		return type->getStructElementType(i);
	else if(type->isVectorTy())
		return type->getVectorElementType();
	else
		return NULL;	
}
	
//Returns the number of primitive elements of type
int //returns
SegmentationTables::getNumPrimitives //Name
(Type* type) //Parameter
{
	//Verifies if this number of primitives was calculated already
	for(int i = 0; i < NumPrimitives.size(); i++)
		if(NumPrimitives[i]->type == type)
			return NumPrimitives[i]->num;
	
	//if not
	int np = 1;
	if(type->isArrayTy())
	{
		int num = type->getArrayNumElements();
		Type* arrtype = type->getArrayElementType();
		int arrtypenum = getNumPrimitives(arrtype); 
		np = num * arrtypenum;
	}
	else if(type->isStructTy())
	{
		int num = type->getStructNumElements();
		np = 0;
		for(int i = 0; i < num; i++)
		{
			Type* structelemtype = type->getStructElementType(i);
			np += getNumPrimitives(structelemtype);
		}
	}
	else if(type->isVectorTy())
	{
		int num = type->getVectorNumElements();
		Type* arrtype = type->getVectorElementType();
		int arrtypenum = getNumPrimitives(arrtype); 
		np = num * arrtypenum;
	}
	
	NumPrimitives.insert(NumPrimitives.end(), new NumPrimitive(type, np));
	return np;
}

//Returns a vector with the primitive layout of type
std::vector<int> //returns
SegmentationTables::getPrimitiveLayout //Name
(Type* type) //Parameter
{
	//Verifies if this layout was calculated already
	for(int i = 0; i < PrimitiveLayouts.size(); i++)
		if(PrimitiveLayouts[i]->type == type)
			return PrimitiveLayouts[i]->layout;
	
	//if not
		
	if(type->isArrayTy())
	{
		int num = type->getArrayNumElements();
		std::vector<int> pm (num);
		Type* arrtype = type->getArrayElementType();
		int arrtypenum = getNumPrimitives(arrtype); 
		for(int i = 0; i < num; i++)
			pm[i] = arrtypenum;
		PrimitiveLayouts.insert(PrimitiveLayouts.end(), new PrimitiveLayout(type, pm));
		return pm;
	}
	else if(type->isStructTy())
	{
		int num = type->getStructNumElements();
		std::vector<int> pm (num);
		for(int i = 0; i < num; i++)
		{
			Type* structelemtype = type->getStructElementType(i);
			pm[i] = getNumPrimitives(structelemtype);
		}
		PrimitiveLayouts.insert(PrimitiveLayouts.end(), new PrimitiveLayout(type, pm));
		return pm;
	}
	else if(type->isVectorTy())
	{
		int num = type->getVectorNumElements();
		std::vector<int> pm (num);
		Type* arrtype = type->getVectorElementType();
		int arrtypenum = getNumPrimitives(arrtype); 
		for(int i = 0; i < num; i++)
			pm[i] = arrtypenum;
		PrimitiveLayouts.insert(PrimitiveLayouts.end(), new PrimitiveLayout(type, pm));
		return pm;
	}
	else
	{
		std::vector<int> pm (1);
		pm[0] = 1;
		PrimitiveLayouts.insert(PrimitiveLayouts.end(), new PrimitiveLayout(type, pm));
		return pm;
	}	
}

/*
* Debug Functions
*/

void //Returns Nothing 
SegmentationTables::printRangeAnalysis //Name
(InterProceduralRA<Cousot> *ra, Module *M) //Parameters
{
	errs() << "Cousot Inter Procedural analysis (Values -> Ranges):\n";
	for(Module::iterator F = M->begin(), Fe = M->end(); F != Fe; ++F)
		for(Function::iterator bb = F->begin(), bbEnd = F->end(); bb != bbEnd; ++bb)
			for(BasicBlock::iterator I = bb->begin(), IEnd = bb->end(); 
			I != IEnd; ++I){
				const Value *v = &(*I);
				Range r = ra->getRange(v);
				if(!r.isUnknown()){
					r.print(errs());
					I->dump();
				}
			}
			
		errs() << "--------------------\n";
}

void //Returns Nothing
SegmentationTables::printRangedPointerMap //Name
() //Parameters
{
	errs() << "\n-------------------------\nMemory Range Map:" << "\n";
	for(llvm::DenseMap<Value*, RangedPointer*>::iterator 
	i =	RangedPointerMap.begin(), e = RangedPointerMap.end(); i != e; i++)
	{
		errs() << "[" ;
		if (i->second->offset.getLower() == Min)
			errs() << "-inf,";
		else
			errs() << i->second->offset.getLower() << ",";
		if (i->second->offset.getUpper() == Max)
			errs() << "+inf";
		else
			errs() << i->second->offset.getUpper();
		errs() << "] -> "<< *(i->first) 
		<< " fathered by " <<  *(i->second->father) << "\n";
	}
	errs() << "-------------------------\n\n";
}

void 
SegmentationTables::printNotUsedPointers
()
{
	errs() << "\n-------------------------\nNot Used Pointers:" << "\n";
	
	for (std::set<Value*>::iterator i = NotUsedPointers.begin(), 
	e = NotUsedPointers.end(); i != e; ++i)
	{
  	errs() << **i << "\n";
  }
	
	
	errs() << "-------------------------\n\n";
}

void //Returns Nothing 
SegmentationTables::printPrimitiveLayouts //Name
(std::vector<PrimitiveLayout*> PrimitiveLayouts) //Parameters
{
	errs() << "\n-------------------------\nPrimitive Layouts:" << "\n";
	for(int i = 0; i < PrimitiveLayouts.size(); i++)
	{
		errs() << *(PrimitiveLayouts[i]->type) << ":\n";
		for(int j = 0; j < PrimitiveLayouts[i]->layout.size(); j++)
			errs() << PrimitiveLayouts[i]->layout[j] << "  ";
		errs() << "\n";
	}
	
	errs() << "-------------------------\n\n";
}

void 
SegmentationTables::printSegmentationTableMap
()
{
	errs() << "\n-------------------------\nRanged Alias Tables:" << "\n";
	
	for (llvm::DenseMap<Value*, SegmentationTable*>::iterator
	i = SegmentationTableMap.begin(), e = SegmentationTableMap.end(); i != e; i++)
	{
  	errs() << "Table: " << *(i->first) << "\n";
  	for(set<SegmentationTableRow*>::iterator ii = i->second->rows.begin(),
  	ee = i->second->rows.end(); ii != ee; ii++)
  	{
  		errs() << "Row: ["; 
  		if((*ii)->offset.getLower() == Min)
  			errs()<< "-inf, ";
  		else
  			errs()<< (*ii)->offset.getLower() << ", ";
  		if((*ii)->offset.getUpper() == Max)
  			errs()<< "+inf]\n";
  		else
  			errs()<< (*ii)->offset.getUpper() << "]\n";
  		for(set<Value*>::iterator iii = (*ii)->pointers.begin(),
  		eee =  (*ii)->pointers.end(); iii != eee; iii++)
  		{
  			errs() << **iii << "\n";
  		}
  	}
  	errs() << "\n";
  }
	
	
	errs() << "-------------------------\n\n";
}

/*
* SegmentationTable support functions
*/

void
SegmentationTable::PrintPointers()
{
	errs() << "\nTable: " << *(base) << "\n";
  for(set<SegmentationTableRow*>::iterator ii = rows.begin(),
  ee = rows.end(); ii != ee; ii++)
  {
  	errs() << "Row: ["; 
  	if((*ii)->offset.getLower() == Min)
  		errs()<< "-inf, ";
  	else
  		errs()<< (*ii)->offset.getLower() << ", ";
  	if((*ii)->offset.getUpper() == Max)
  		errs()<< "+inf]\n";
  	else
  		errs()<< (*ii)->offset.getUpper() << "]\n";
  	for(set<Value*>::iterator iii = (*ii)->pointers.begin(), eee =  (*ii)->pointers.end(); iii != eee; iii++)
  	{
  		errs() << **iii << "\n";
  	}
  }
  errs() << "\n";
}

void
SegmentationTable::PrintContent()
{
	errs() << "\nTable: " << *(base) << "\n";
  for(set<SegmentationTableRow*>::iterator ii = rows.begin(),
  ee = rows.end(); ii != ee; ii++)
  {
  	errs() << "Row: ["; 
  	if((*ii)->offset.getLower() == Min)
  		errs()<< "-inf, ";
  	else
  		errs()<< (*ii)->offset.getLower() << ", ";
  	if((*ii)->offset.getUpper() == Max)
  		errs()<< "+inf] - ";
  	else
  		errs()<< (*ii)->offset.getUpper() << "] - ";
  	
  	errs() << "["; 
  	if((*ii)->content.getLower() == Min)
  		errs()<< "-inf, ";
  	else
  		errs()<< (*ii)->content.getLower() << ", ";
  	if((*ii)->content.getUpper() == Max)
  		errs()<< "+inf]\n";
  	else
  		errs()<< (*ii)->content.getUpper() << "]\n";
  }
  errs() << "\n";
}

/*
* SegmentationTables support functions
*/

llvm::DenseMap<Value*, RangedPointer*> 
SegmentationTables::getRangedPointerMap
()
{
	return RangedPointerMap;
}

std::set<Value*> 
SegmentationTables::getNotUsedPointers
()
{
	return NotUsedPointers;
}

llvm::DenseMap<Value*, std::set<RangedPointer*> > 
SegmentationTables::getRangedPointerSets
()
{
	return RangedPointerSets;
}

llvm::DenseMap<Value*, SegmentationTable*> 
SegmentationTables::getSegmentationTableMap
()
{
	return SegmentationTableMap;
}

/*
* LLVM framework support functions
*/

void //Returns nothing
SegmentationTables::getAnalysisUsage //Name
(AnalysisUsage &AU) const //Parameters
{
	AU.setPreservesAll();
	AU.addRequired<InterProceduralRA<Cousot> >();
}
char SegmentationTables::ID = 0;
static RegisterPass<SegmentationTables> X("segmentation-tables",
"Get tables memory ranges and pointers that can point for such ranges", false, false);

/*
*
* LLVM framework Main Function
*		Does the main analysis
*
*/

bool //Returns 
SegmentationTables::runOnModule //Name
(Module &M) //Parameters
{
	/*
	* Gets RangeAnalysis Pass Data, declares some auxiliary sets and initiates statistics.
	*/
	
	InterProceduralRA<Cousot> &ra = getAnalysis<InterProceduralRA<Cousot> >();
	unsigned MaxBitWidth = getMaxBitWidth(M);
	DEBUG(printRangeAnalysis(&ra, &M));
	
	std::set<Value*> AlocSet;
	std::set<Value*> GlobalSet;
	std::set<Value*> ParameterSet;
	std::set<Value*> LoadSet;

	NATables = 0;
	NParameter = 0;
	NLoad = 0;
	NGlobal = 0;
	
	DEBUG(errs() << "-inf:" << Min << "\n+inf:" << Max << "\n");
	
	/*
	*	Go through global variables to find arrays, structs and pointers
	*/
	
	for(Module::global_iterator i = M.global_begin(), e = M.global_end();
	i != e; i++)
	{
		
		RangedPointerMap[i] = new RangedPointer(i, Zero, Zero, i, NULL);
	  AlocSet.insert(i);
	  GlobalSet.insert(i);
	  NGlobal++;
	  NATables++;
	}
	
	/*
	* Go through all functions from the module
	*/
	
	for (Module::iterator F = M.begin(), Fe = M.end(); F != Fe; F++)
	{
		
		/*
		* See if the parameters can have tables (are pointers)
		*/
		for(Function::arg_iterator i = F->arg_begin(), e = F->arg_end();
		i != e; i++)
		{
			Type* arg_type = i->getType();
			if(arg_type->isPointerTy())
			{
				//pointer Range [0,0]
	   		RangedPointerMap[i] = new RangedPointer(i, Zero, Zero, i, F);
	   		AlocSet.insert(i);
	   		ParameterSet.insert(i);
	   		NParameter++;
	   		NATables++;
			}
		}
		
		/*
		* Run through instructions from function
		* calculating memory ranges
		*/
		for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I)
		{
			Instruction* i = &(*I);
			
			//Tries to indetify the instruction and do proper calculations
     	if(isa<AllocaInst>(*I))
     	{
	   		//pointer Range [0,0]
	   		RangedPointerMap[i] = new RangedPointer(i, Zero, Zero, i, F);
	   		AlocSet.insert(i);
	   		NATables++;
      }
      else if(isa<CallInst>(*I))
			{
   			Function* cf = ((CallInst*)i)->getCalledFunction();
   			if(cf)
   			{
   				Type* call_type = i->getType();
   				if(call_type->isPointerTy())
   				{
   					RangedPointerMap[i] = new RangedPointer(i, Zero, Zero, i, F);
	   				AlocSet.insert(i);
	   				NATables++;
   				}
   			}
			}
			else if(isa<BitCastInst>(*I))
      {
       	Value* base_ptr = i->getOperand(0);
       	DenseMap<Value*, RangedPointer*>::iterator base_it = RangedPointerMap.find(base_ptr);
       	if(base_it == RangedPointerMap.end())
       	{
       		NotUsedPointers.insert(i);
       		continue;
       	}
       	RangedPointer* base_range = base_it->second;
       	RangedPointerMap[i] = 
       		new RangedPointer(i,base_range->offset.getLower(),base_range->offset.getUpper(), base_range->father, F);
      }
      else if(isa<LoadInst>(*I) && I->getType()->isPointerTy())
      {
      	RangedPointerMap[i] = new RangedPointer(i, Zero, Zero, i, F);
	   		AlocSet.insert(i);
	   		LoadSet.insert(i);
	   		NLoad++;
	   		NATables++;
      }
      else if(isa<PHINode>(*I) && I->getType()->isPointerTy())
      {
      	RangedPointerMap[i] = new RangedPointer(i, Zero, Zero, i, F);
	   		AlocSet.insert(i);
	   		NATables++;
      }
      else if(isa<GetElementPtrInst>(*I))//GEP processing
			{	
				//pointer Range basePtrRangedPointer + indexes range
				Value* base_ptr = ((GetElementPtrInst*)i)->getPointerOperand();
      	DenseMap<Value*, RangedPointer*>::iterator base_it = RangedPointerMap.find(base_ptr);
       	if(base_it == RangedPointerMap.end())
       	{
       		NotUsedPointers.insert(i);
       		continue;
       	}
       	//It was possible to find the base pointer of this GEP
       	RangedPointer* base_range = base_it->second;
       	
       	APInt lower_range;
       	APInt higher_range;
       	lower_range = base_range->offset.getLower();
       	higher_range = base_range->offset.getUpper();
       	
       	//Number of primitive elements
       	Type* base_ptr_type = base_ptr->getType();
       	int base_ptr_num_primitive = getNumPrimitives(base_ptr_type->getPointerElementType());
       	
       	//parse first index
       	User::op_iterator idx = ((GetElementPtrInst*)i)->idx_begin();
       	Value* indx = idx->get();
       	
       	if(isa<ConstantInt>(*indx))
        {
         	APInt constant = ((ConstantInt*)indx)->getValue();
        	int higher_bitwidth = (lower_range.getBitWidth() > constant.getBitWidth()) ?
        		lower_range.getBitWidth() :
        		constant.getBitWidth();
        	higher_bitwidth = (higher_range.getBitWidth() > higher_bitwidth) ?
        		higher_range.getBitWidth() :
        		higher_bitwidth;
        		
        	APInt bpnp (higher_bitwidth, base_ptr_num_primitive);
        	
        	lower_range = lower_range.sextOrTrunc(higher_bitwidth);
        	higher_range = higher_range.sextOrTrunc(higher_bitwidth);
        	constant = constant.sextOrTrunc(higher_bitwidth);
        	
					lower_range = safe_addition(lower_range, safe_multiplication(bpnp, constant));
					higher_range = safe_addition(higher_range, safe_multiplication(bpnp, constant));
					        	
        }
        else
        {
        	Range r = ra.getRange(indx);
        	if(!r.isUnknown())
         	{
          	APInt rl = r.getLower();
        		APInt ru = r.getUpper();
          	
          	int higher_bitwidth = (lower_range.getBitWidth() > rl.getBitWidth()) ?
        			lower_range.getBitWidth() :
        			rl.getBitWidth();
        		higher_bitwidth = (higher_range.getBitWidth() > higher_bitwidth) ?
        			higher_range.getBitWidth() :
        			higher_bitwidth;
        			
        		APInt bpnp (higher_bitwidth, base_ptr_num_primitive);
        	
        		lower_range = lower_range.sextOrTrunc(higher_bitwidth);
        		higher_range = higher_range.sextOrTrunc(higher_bitwidth);
        		rl = rl.sextOrTrunc(higher_bitwidth);
        		ru = ru.sextOrTrunc(higher_bitwidth);
          
          	lower_range = safe_addition(lower_range, safe_multiplication(bpnp, rl));
						higher_range = safe_addition(higher_range, safe_multiplication(bpnp, ru));
          }
        }
        
        //parse sequantial indexes 
       	idx++;
       	APInt index = Zero;
       	
       	for(User::op_iterator idxe = ((GetElementPtrInst*)i)->idx_end(); idx != idxe; idx++)
       	{
       		//Calculating Primitive Layout
       		base_ptr_type = getTypeInside(base_ptr_type, index.getSExtValue());
     			std::vector<int> base_ptr_primitive_layout = getPrimitiveLayout(base_ptr_type);
     			
     			Value* indx = idx->get();
        	if(isa<ConstantInt>(*indx))
        	{
        		APInt constant = ((ConstantInt*)indx)->getValue();
        		int higher_bitwidth = (lower_range.getBitWidth() > constant.getBitWidth()) ?
        			lower_range.getBitWidth() :
        			constant.getBitWidth();
        		higher_bitwidth = (higher_range.getBitWidth() > higher_bitwidth) ?
        			higher_range.getBitWidth() :
        			higher_bitwidth;
		      			
		      	APInt bpnp (higher_bitwidth, base_ptr_num_primitive);
		      	
		      	lower_range = lower_range.sextOrTrunc(higher_bitwidth);
		      	higher_range = higher_range.sextOrTrunc(higher_bitwidth);
		      	constant = constant.sextOrTrunc(higher_bitwidth);
		      	
						lower_range = safe_addition( lower_range, APInt(higher_bitwidth, getSumBehind(base_ptr_primitive_layout, constant.getSExtValue()) ) );
						higher_range = safe_addition( higher_range, APInt(higher_bitwidth, getSumBehind(base_ptr_primitive_layout, constant.getSExtValue()) ) );

		      	index = constant;
        	}
        	else
        	{
        		Range r = ra.getRange(indx);
		      	if(!r.isUnknown())
		       	{
		        	APInt rl = r.getLower();
		      		APInt ru = r.getUpper();
		        	
		        	int higher_bitwidth = (lower_range.getBitWidth() > rl.getBitWidth()) ?
		      			lower_range.getBitWidth() :
		      			rl.getBitWidth();
		      		higher_bitwidth = (higher_range.getBitWidth() > higher_bitwidth) ?
		      			higher_range.getBitWidth() :
		      			higher_bitwidth;
		      			
		      		APInt bpnp (higher_bitwidth, base_ptr_num_primitive);
		      	
		      		lower_range = lower_range.sextOrTrunc(higher_bitwidth);
		      		higher_range = higher_range.sextOrTrunc(higher_bitwidth);
		      		rl = rl.sextOrTrunc(higher_bitwidth);
		      		ru = ru.sextOrTrunc(higher_bitwidth);
		        
		        	lower_range = safe_addition( lower_range, APInt(higher_bitwidth, getSumBehind(base_ptr_primitive_layout, rl.getSExtValue()) ) );
							higher_range = safe_addition( higher_range, APInt(higher_bitwidth, getSumBehind(base_ptr_primitive_layout, ru.getSExtValue()) ) );
		        
		        	index = Zero;
		    	  }
		       		
        	}
       	}
       	
       	RangedPointerMap[i] = new RangedPointer(i, lower_range, higher_range, base_range->father, F);
				
			}//end of GEP processing
      else
			{
				Type *non_type = i->getType();
				if(non_type->isPointerTy())
					NotUsedPointers.insert(i);
			}
		}//end of function's instruction loop
	}//end of all functions loop
	
	DEBUG(printRangedPointerMap());
	DEBUG(printNotUsedPointers());
	DEBUG(printPrimitiveLayouts(PrimitiveLayouts));
	
	/*
	* Separating pointers for table construction
	*/
	
	for(std::set<Value*>::iterator i = AlocSet.begin(),
	e = AlocSet.end(); i != e; i++)
	{
		for(llvm::DenseMap<Value*, RangedPointer*>::iterator ii = RangedPointerMap.begin(),
		ee = RangedPointerMap.end(); ii != ee; ii++)
		{
			if(*i == ii->second->father)
				RangedPointerSets[*i].insert(ii->second);
		}
	}
	
	/*
	*	Building Tables
	*/
	
	//for each interesting set of pointers
	for (llvm::DenseMap<Value*, std::set<RangedPointer*> >::iterator 
	i = RangedPointerSets.begin(), e = RangedPointerSets.end(); i != e; ++i)
	{
		APInt MinusOne = Zero;
		MinusOne--;
		APInt One = Zero;
		One++;
		APInt lower_range = Min;
		APInt higher_range = Min;
		
		SegmentationTableMap[i->first] = new SegmentationTable();
		SegmentationTableMap[i->first]->row_num = 0;
		SegmentationTableMap[i->first]->ranged_pointers = i->second;
		
		//adding entries to the table
		while(true)
		{
			/////////////////////////////
			//Calculating new lower range
			
			APInt new_lower_range = Min;
			
			//if lower_range hasn't been calculated before than we should find
			//the lowest lower range, if not the next lower range is higher+1
			if(higher_range != Min)
			{
				new_lower_range = higher_range + One;
			}
			
			//////////////////////////////
			//Calculating new higher range
			
			APInt new_higher_range1 = Max;
			APInt new_higher_range2 = Max;
			
			for (std::set<RangedPointer*>::iterator ii = i->second.begin(), 
			ee = i->second.end(); ii != ee; ++ii)
	    {
	    	if((*ii)->offset.getUpper().slt(new_higher_range1) and (*ii)->offset.getUpper().sgt(higher_range))
	    		new_higher_range1 = (*ii)->offset.getUpper();
	    	
	    	if((*ii)->offset.getLower().slt(new_higher_range2) and (*ii)->offset.getLower().sgt(new_lower_range))
	    		new_higher_range2 = (*ii)->offset.getLower() - One;
	    }
			
			//refreshing ranges
			lower_range = new_lower_range;
			higher_range = new_higher_range1.slt(new_higher_range2) ? new_higher_range1 : new_higher_range2;
			
			////////////////////
			//Adding row members
	    SegmentationTableRow* table_row = new SegmentationTableRow();
	    table_row->offset.setLower(lower_range);
	    table_row->offset.setUpper(higher_range);
	    
	    bool mrk = false;
	    for (std::set<RangedPointer*>::iterator ii = i->second.begin(), ee = i->second.end(); 
      ii != ee; ++ii)
      {
      	if((*ii)->offset.getLower().sle(higher_range) and (*ii)->offset.getUpper().sge(lower_range))
      	{
      		table_row->pointers.insert((*ii)->pointer);		
      		SegmentationTableMap[i->first]->pointer_to_rows[(*ii)->pointer].insert(table_row);
      		mrk = true;
      	}
      }
			
			/////////////////////
			//finishing loop turn
			SegmentationTableMap[i->first]->rows.insert(table_row);
      if(mrk == true) SegmentationTableMap[i->first]->row_num++;
      
      //evaluate break point
      if(higher_range == Max)
      	break;      
    }
	  
	}
	DEBUG(printSegmentationTableMap());

	/*
	* Filling tables associations and statistics
	*/	
	
	NTables = 0;
	NTableAvgSize = 0;
	NTableMaxSize = 0;
	PointersUsed = 0;
	NStruct = 0;
	NArray = 0;
	NVector = 0;
		
	for(DenseMap<Value*, SegmentationTable*>::iterator i = SegmentationTableMap.begin(),
	e = SegmentationTableMap.end(); i != e; i++)
	{
		i->second->has_no_alias = false;		
		i->second->base = i->first;
		i->second->function = RangedPointerMap[i->first]->func;
		if(isa<AllocaInst>(*(i->first)))
    {
	   	i->second->aloc_base = true;
    }
		else if(isa<CallInst>(*(i->first)))
		{
   		Function* cf = ((CallInst*)i->first)->getCalledFunction();
   			if(cf)
   				if( strcmp( cf->getName().data(), "malloc") == 0 
   				or strcmp( cf->getName().data(), "calloc") == 0	)
   				{
	 					i->second->aloc_base = true;
					}
		}
		else
		{
			i->second->aloc_base = false;
		}
		
		int tsize = i->second->row_num;
		if(tsize > 1)
		{
			NTables++;
			NTableAvgSize += tsize;
			if (tsize > NTableMaxSize)
				NTableMaxSize = tsize; 
		}
		
		//check table's type
		i->second->is_struct = false;
		i->second->is_array = false;
		i->second->is_vector = false;
		i->second->is_parameter = false;
		i->second->is_load = false;
		i->second->is_global = false;
		
		Type* table_type = i->first->getType();
		if(table_type->isPointerTy())
			table_type = table_type->getPointerElementType();
	
		if(table_type->isArrayTy())
		{
			i->second->is_array = true;
			NArray++;
		}
		else if(table_type->isStructTy())
		{
			i->second->is_struct = true;
			NStruct++;
		}
		else if(table_type->isVectorTy())
		{
			i->second->is_vector = true;
			NVector++;
		}
		
		if(ParameterSet.find(i->first) != ParameterSet.end())
			i->second->is_parameter = true;
		else if(GlobalSet.find(i->first) != GlobalSet.end())
		{
			i->second->is_global = true;
			if(!i->first->getType()->getPointerElementType()->isPointerTy())
				i->second->aloc_base = true;
		}
		else if(LoadSet.find(i->first) != LoadSet.end())
			i->second->is_load = true;
	}
	if(NTables > 0)
		NTableAvgSize /= NTables;
	int npointers = RangedPointerMap.size();
	PointersUsed = (npointers*100)/(npointers + NotUsedPointers.size());

	return false;
}
