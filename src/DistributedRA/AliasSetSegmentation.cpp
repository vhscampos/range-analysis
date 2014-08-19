/*
 * AliasSetSegmentation.cpp
 *
 *  Created on: Jun 5, 2014
 *      Author: raphael
 */
#define DEBUG_TYPE "as-segmentation"
#include "AliasSetSegmentation.h"

using namespace llvm;

//statistics
STATISTIC(NSegments, "Number of relevant segments");
STATISTIC(NDefined, "Number of relevant segments with well defined content range");
STATISTIC(NAvgSegments, "Average of relevant segments per alias sets table");
STATISTIC(NMaxSegments, "Maximum of relevant segments per alias sets table");


//Global Variables Declarations
extern APInt Min;
extern APInt Max;
extern APInt Zero;

bool AliasSetSegmentation::intersection(Range i, Range j)
{
	APInt il = i.getLower();
	APInt iu = i.getUpper();
	APInt jl = j.getLower();
	APInt ju = j.getUpper();
	
	if(il.sle(ju))
	{
		if(iu.sge(jl))
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

/*
* Merge functions
*/

void AliasSetSegmentation::ContentMerge (SegmentationTable* result, SegmentationTable* one, SegmentationTable* two)
{
	//for each row of result
	for(std::set<SegmentationTableRow*>::iterator i = result->rows.begin(), e = result->rows.end(); i != e; i++)
	{
		SegmentationTableRow* result_row = *i;
		//make its content Unknown
		result_row->content.setUnknown();
		//for each row of one
		for(std::set<SegmentationTableRow*>::iterator ii = one->rows.begin(), ee = one->rows.end(); ii != ee; ii++)
		{
			SegmentationTableRow* one_row = *ii;
			//if rows have intersection
			if( intersection(result_row->offset, one_row->offset) )
			{
				//union with content
				result_row->content = result_row->content.unionWith(one_row->content);
			}
		}		
		//for each row of two
		for(std::set<SegmentationTableRow*>::iterator ii = two->rows.begin(), ee = two->rows.end(); ii != ee; ii++)
		{
			SegmentationTableRow* two_row = *ii;
			//if rows have intersection
			if( intersection(result_row->offset, two_row->offset) )
			{
				//union with content
				result_row->content = result_row->content.unionWith(two_row->content);
			}
		}
	}	
}

bool AliasSetSegmentation::CommonPointers(SegmentationTable* one, SegmentationTable* two)
{
	for(std::set<RangedPointer*>::iterator i = one->ranged_pointers.begin(), e = one->ranged_pointers.end(); i != e; i++)
		for(std::set<RangedPointer*>::iterator ii = two->ranged_pointers.begin(), ee = two->ranged_pointers.end(); ii != ee; ii++)
			if( (*i)->pointer == (*ii)->pointer )
				return true;
	return false;
}

void AliasSetSegmentation::BuildTable(SegmentationTable* result)
{
	APInt MinusOne = Zero;
	MinusOne--;
	APInt One = Zero;
	One++;
	APInt lower_range = Min;
	APInt higher_range = Min;
	result->row_num = 0;
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
		
		for (std::set<RangedPointer*>::iterator ii = result->ranged_pointers.begin(), 
		ee = result->ranged_pointers.end(); ii != ee; ++ii)
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
    for (std::set<RangedPointer*>::iterator ii = result->ranged_pointers.begin(), ee = result->ranged_pointers.end(); 
    ii != ee; ++ii)
    {
     	if((*ii)->offset.getLower().sle(higher_range) and (*ii)->offset.getUpper().sge(lower_range))
     	{
     		table_row->pointers.insert((*ii)->pointer);		
     		result->pointer_to_rows[(*ii)->pointer].insert(table_row);
     		mrk = true;
     	}
    }
			
		/////////////////////
		//finishing loop turn
		result->rows.insert(table_row);
    if(mrk == true) result->row_num++;
     
    //evaluate break point
    if(higher_range == Max)
     	break;      
  }
}

SegmentationTable* AliasSetSegmentation::OneLineTransformation (SegmentationTable* Table)
{
	SegmentationTable* result = new SegmentationTable();
	//go through tables ranged pointers adding them to the result
	for(std::set<RangedPointer*>::iterator i = Table->ranged_pointers.begin(), e = Table->ranged_pointers.end(); i != e; i++)
		result->ranged_pointers.insert( new RangedPointer((*i)->pointer, Min, Max, (*i)->father, (*i)->func) );
	//building result table
	BuildTable(result);
	
	//passing associations
	result->base = Table->base;
	result->aloc_base = Table->aloc_base;
	result->function = Table->function;
	result->recursive_func = Table->recursive_func;
	result->AliasSet = Table->AliasSet;
	result->full_alias_set = Table->full_alias_set;
	result->has_no_alias = false;
	result->is_struct = Table->is_struct;
	result->is_array = Table->is_array;
	result->is_vector = Table->is_vector;
	result->is_parameter = Table->is_parameter;
	result->is_load = Table->is_load;
	result->is_global = Table->is_global;
	
	return result;
}

SegmentationTable* AliasSetSegmentation::UnionMerge (SegmentationTable* one, SegmentationTable* two)
{
	SegmentationTable* result = new SegmentationTable();
	//go through tables ranged pointers adding them to the result
	for(std::set<RangedPointer*>::iterator i = one->ranged_pointers.begin(), e = one->ranged_pointers.end(); i != e; i++)
		result->ranged_pointers.insert(*i);
	for(std::set<RangedPointer*>::iterator i = two->ranged_pointers.begin(), e = two->ranged_pointers.end(); i != e; i++)
		result->ranged_pointers.insert(*i);
	//building result table
	BuildTable(result);
  
	//passing associations
	result->base = NULL;
	result->aloc_base = one->aloc_base;
	if(one->function == two->function)
	{
		result->function = one->function;
		result->recursive_func = one->recursive_func;
	}
	else
	{
		result->function = NULL;
	}
	if(one->AliasSet == two->AliasSet)
		result->AliasSet = one->AliasSet;
	
	
  return result;
}

SegmentationTable* 
AliasSetSegmentation::OffsetMerge 
(SegmentationTable* one, SegmentationTable* two, Range Offset)
{
	SegmentationTable* result = new SegmentationTable();
	//go through tables ranged pointers adding them to the result
	for(std::set<RangedPointer*>::iterator i = one->ranged_pointers.begin(), e = one->ranged_pointers.end(); i != e; i++)
		result->ranged_pointers.insert(*i);
	for(std::set<RangedPointer*>::iterator i = two->ranged_pointers.begin(), e = two->ranged_pointers.end(); i != e; i++)
	{
		Range new_offset = (*i)->offset.add(Offset);
		result->ranged_pointers.insert( new RangedPointer((*i)->pointer, new_offset.getLower(), new_offset.getUpper(), (*i)->father, (*i)->func) );
	}
	//building result table
	BuildTable(result);
    
	//passing associations
	result->base = one->base;
	result->aloc_base = one->aloc_base;
	result->function = one->function;
	result->recursive_func = one->recursive_func;
	result->AliasSet = one->AliasSet;
	result->full_alias_set = one->full_alias_set;
	result->has_no_alias = one->has_no_alias;
	result->is_struct = one->is_struct;
	result->is_array = one->is_array;
	result->is_vector = one->is_vector;
	result->is_parameter = one->is_parameter;
	result->is_load = one->is_load;
	result->is_global = one->is_global;
	
  return result;
}

SegmentationTable* 
AliasSetSegmentation::MultiplicationMerge 
(SegmentationTable* one, SegmentationTable* two)
{
	SegmentationTable* result = new SegmentationTable();
	//go through tables ranged pointers adding them to the result
	for(std::set<RangedPointer*>::iterator i = one->ranged_pointers.begin(), e = one->ranged_pointers.end(); i != e; i++)
		result->ranged_pointers.insert(*i);
	for(std::set<RangedPointer*>::iterator i = two->ranged_pointers.begin(), e = two->ranged_pointers.end(); i != e; i++)
		result->ranged_pointers.insert( new RangedPointer((*i)->pointer, Min, Max, (*i)->father, (*i)->func) );
	//building result table
	BuildTable(result);
    
	//passing associations
	result->base = one->base;
	result->aloc_base = one->aloc_base;
	result->function = one->function;
	result->recursive_func = one->recursive_func;
	result->AliasSet = one->AliasSet;
	result->full_alias_set = one->full_alias_set;
	result->has_no_alias = one->has_no_alias;
	result->is_struct = one->is_struct;
	result->is_array = one->is_array;
	result->is_vector = one->is_vector;
	result->is_parameter = one->is_parameter;
	result->is_load = one->is_load;
	result->is_global = one->is_global;
	
  return result;
}

/*
*
* LLVM framework Main Function
*		Does the main analysis
*
*/

bool AliasSetSegmentation::runOnModule(Module &M){

	//Here we compute the big tables per AliasSet

	//get ranges, tables and alias sets
	SegmentationTables &st = getAnalysis<SegmentationTables >();
	llvm::DenseMap<Value*, SegmentationTable*> stm = st.getSegmentationTableMap();
	AliasSets &as = getAnalysis<AliasSets >();
	llvm::DenseMap<int, std::set<Value*> > vs = as.getValueSets();
	InterProceduralRA<Cousot> &ra = getAnalysis<InterProceduralRA<Cousot> >();
	
																								
																								/** ///testing alias sets
																								for (llvm::DenseMap<int, std::set<Value*> >::iterator i = vs.begin(), e = vs.end(); i != e; ++i) 
																							 	{
																										errs() << "Set " << i->first << " :\n";
																										for (std::set<Value*>::iterator ii = i->second.begin(), ee = i->second.end(); ii != ee; ++ii) 
																										{
																												errs() << "	" << **ii << "\n";
																										}
																										errs() << "\n";
																								}/**/
	
	//
	//Associating Alias Set to each table
	errs() << "ASSOCIATING ALIAS SETS FOR EACH TABLE\n";
 	//
  for(DenseMap<Value*, SegmentationTable*>::iterator i = stm.begin(), e = stm.end(); i != e; i++)
  {
  	int akey = as.getValueSetKey(i->first);
		if(akey > 0)
		{
			i->second->AliasSet = &(vs[akey]);
			tables_sets[akey].insert(i->second);
		}
	}
	
	//Creating loose pointers and connections map
	llvm::DenseMap<int, std::set<Value*> > loose_pointers;
	llvm::DenseMap<Value*, std::set<Value*> > connections;
	
	//
	//finding connections        
	errs() << "FINDING CONNECTIONS\n";
	//
		//for each alias sets  
	for(llvm::DenseMap<int, std::set<Value*> >::iterator i = vs.begin(), e = vs.end(); i != e; i++)
	{
		for(std::set<SegmentationTable*>::iterator ii = tables_sets[i->first].begin(), ee = tables_sets[i->first].end(); ii != ee; ii++)
		{
			SegmentationTable* table = *ii;
			Value* base = table->base;
			//if the base is a parameter, then we must find function calls
			if(table->aloc_base)
			{
				continue;
			}
			else if(table->is_parameter)
			{
				Function* f = table->function;
				Argument* a = (Argument*) table->base;
				
				for(llvm::Value::use_iterator ui = f->use_begin(), ue = f->use_end(); ui != ue; ui++)
				{	
					User* u = *ui;
					if(isa<CallInst>(*u))
					{
						int anum = ((CallInst*) u)->getNumArgOperands();
						int ano = a->getArgNo();
						if(ano <= anum)
							connections[base].insert(u->getOperand(a->getArgNo()) );
						else
						{
							errs() << "ERROR!!!\n";
							errs() << *a << " " << ano << "\n";
							errs() << *u << "\n";
							return false;
						}
					}	
				}
			}
			else if(table->is_global)
			{
				for(std::set<RangedPointer*>::iterator pi = table->ranged_pointers.begin(), pe = table->ranged_pointers.end(); pi != pe; pi++)
					loose_pointers[i->first].insert((*pi)->pointer);
			}
			else if(isa<PHINode>(*base))
			{
				unsigned int num = ((PHINode*)base)->getNumIncomingValues();
				for (int ind = 0; ind < num; ind++)
					connections[base].insert( ((PHINode*)base)->getIncomingValue(ind) );
			}
			else if(isa<LoadInst>(*base))
			{
				for(std::set<RangedPointer*>::iterator pi = table->ranged_pointers.begin(), pe = table->ranged_pointers.end(); pi != pe; pi++)
					loose_pointers[i->first].insert((*pi)->pointer);
			}
			else if(isa<CallInst>(*base))
			{
				Function* f = ((CallInst*)base)->getCalledFunction();
				for (inst_iterator I = inst_begin(f), E = inst_end(f); I != E; ++I)
					if(isa<ReturnInst>(*base))
						connections[base].insert( ((ReturnInst*)base)->getReturnValue() );
			}
			else
			{
				for(std::set<RangedPointer*>::iterator pi = table->ranged_pointers.begin(), pe = table->ranged_pointers.end(); pi != pe; pi++)
					loose_pointers[i->first].insert((*pi)->pointer);
			}
			
		}
	}
	
																														/** ///testing connections
																														errs() << "CONNECTIONS\n-------------------\n\n";
																														for(llvm::DenseMap<Value*, std::set<Value*> >::iterator i = connections.begin(), e = connections.end(); i != e; i++)
																														{
																															errs() << *(i->first) << "\n  Connects with:\n";
																															for(std::set<Value*>::iterator ii = i->second.begin(), ee = i->second.end(); ii != ee; ii++)
																																errs() << **ii << "\n";
																															errs() << "\n";
																														}/**/
	
																														/** ///testing loose pointers
																														errs() << "\nLOOSE POINTERS\n-------------------\n";
																														for(llvm::DenseMap<int, std::set<Value*> >::iterator i = loose_pointers.begin(), e = loose_pointers.end(); i != e; i++)
																														{
																															errs() << "Loose pointers of set " << (i->first) << " :\n";
																															for(std::set<Value*>::iterator ii = i->second.begin(), ee = i->second.end(); ii != ee; ii++)
																																errs() << **ii << "\n";
																															errs() << "\n";
																														}/**/
	
	//
	// CONVERGENCE
	//resolve necessary merge operations
	errs() << "STARTING TABLE CONVERGENCE\n";
	//int num_tables = 0;
	//
		//for each alias set
	for(llvm::DenseMap<int, std::set<Value*> >::iterator i = vs.begin(), e = vs.end(); i != e; i++)
	{
		//first you create a set of tables with all tables of the alias set
		std::set<SegmentationTable*> tables_to_merge;
		tables_to_merge = tables_sets[i->first];

//																														/**///testing convergence
//																														errs() << "\nCONVERGENCE OF SET " << i->first;
//																														errs() << "\n--------------\n";
//																														for(std::set<SegmentationTable*>::iterator ti = tables_to_merge.begin(), te = tables_to_merge.end(); ti != te; ti++)
//																														{
//																															SegmentationTable* table = *ti;
//																															//table->PrintPointers();
//																															if(table->aloc_base) errs() << "1 ";
//																															else errs() << "0 ";
//																														}
//																														errs() << " - INICIAL\n";
//																														/**/

		//enter a loop that goes on utill its not possible to merge any table
		while(true)
		{
			bool stop = true;

			std::set<SegmentationTable*> tables_to_remove;
			std::set<SegmentationTable*> tables_to_add;
			
			SegmentationTable* table2 = NULL;
			SegmentationTable* table1 = NULL;
				
			//finds a table, from the set, with connections by going through the set
			for(std::set<SegmentationTable*>::iterator ti = tables_to_merge.begin(), te = tables_to_merge.end(); ti != te; ti++)
			{
				SegmentationTable* table = *ti;
				Value* base = table->base;
				//if a table has no connections and is not an allocation table, then add its pointers to loose and remove it from the set 
				if(connections[base].empty())
				{
					if(table->aloc_base) continue;
					for(std::set<RangedPointer*>::iterator pi = table->ranged_pointers.begin(), pe = table->ranged_pointers.end(); pi != pe; pi++)
					{
						if( loose_pointers[i->first].find((*pi)->pointer) == loose_pointers[i->first].end() )
							loose_pointers[i->first].insert((*pi)->pointer);
					}
					tables_to_remove.insert(table);
					continue;
				}
				//if it has connections, use it
				table2 = table;
				break;
			}
			for(std::set<SegmentationTable*>::iterator ti = tables_to_remove.begin(), te = tables_to_remove.end(); ti != te; ti++)
			{
				tables_to_merge.erase(*ti);
				delete (*ti);
			}
			tables_to_remove.clear();
			//If no table with connections was found, stop automatically
			if(table2 == NULL)break; 
						
			//for each connection 
			for(std::set<Value*>::iterator ci = connections[table2->base].begin(), ce = connections[table2->base].end(); ci != ce; ci++)
			{
				Value* connected = *ci;
				//find the tables with the connected pointer
				bool loose_connection = true;
				std::set<SegmentationTable*> tables_connected;
				for(std::set<SegmentationTable*>::iterator tii = tables_to_merge.begin(), tee = tables_to_merge.end(); tii != tee; tii++)
				{
					SegmentationTable* table = *tii;
					RangedPointer* rp = RangedPointer::FindByValue(connected, table->ranged_pointers);
					if(rp == NULL)continue; //is not in this table
					//is in the table
					tables_connected.insert(table);
					loose_connection = false;
				}
				//if a connection goes to a pointer not in a table, put pointers of that table in loose pointers, and go to the next connection |don't stop
				if(loose_connection)
				{
					for(std::set<RangedPointer*>::iterator pi = table2->ranged_pointers.begin(), pe = table2->ranged_pointers.end(); pi != pe; pi++)
					{
						if( loose_pointers[i->first].find((*pi)->pointer) == loose_pointers[i->first].end() )
							loose_pointers[i->first].insert((*pi)->pointer);
					}
					stop = false;
					continue;
				}
				//for each table do apropriate merges and operations
				for(std::set<SegmentationTable*>::iterator tii = tables_connected.begin(), tee = tables_connected.end(); tii != tee; tii++)
				{
					SegmentationTable* table1 = *tii;
					//if the tables are the same and it has more than one line
					if(table1 == table2)
					{
						//errs() << "SAME\n";
						if(table1->row_num > 1)
						{
							//errs()<<"ONELINETRANS\n";
							//OneLineTransformation into tables_to_add |don't stop
							tables_to_add.insert(OneLineTransformation(table1));
							stop = false;
						}
					}
					//if they are diferent
					else
					{
						//errs() << "DIFF\n";
						//if they have common pointers, MultiplicationMerge into tables_to_add and add table1 to tables_to_remove |don't stop
						if(CommonPointers(table1, table2))
						{
							//errs() << "MULTI\n";
							tables_to_add.insert(MultiplicationMerge(table1, table2));
							tables_to_remove.insert(table1);
							stop = false;
						}
						//if they don't have common pointers, OffsetMerge into tables_to_add and add table1 to tables_to_remove |don't stop
						else
						{
							//errs() << "OFFSET\n";
							RangedPointer* rp = RangedPointer::FindByValue(connected, table1->ranged_pointers);
							tables_to_add.insert(OffsetMerge(table1, table2, rp->offset));
							tables_to_remove.insert(table1);
							stop = false;
						}
					}
				}
			}
			//add table2 to tables_to_remove
			tables_to_remove.insert(table2);
			//remove from set tables_to_remove and add to set tables_to_add
			for(std::set<SegmentationTable*>::iterator ti = tables_to_remove.begin(), te = tables_to_remove.end(); ti != te; ti++)
			{
				tables_to_merge.erase(*ti);
				delete (*ti);
				//errs() << "ERASING\n";
			}
			for(std::set<SegmentationTable*>::iterator ti = tables_to_add.begin(), te = tables_to_add.end(); ti != te; ti++)
			{
				tables_to_merge.insert(*ti);
				//errs() << "ADDING\n";
			}
			//final step - union merge tables with the same parent
			/**/
			bool m;
			do{
				m = false;
				SegmentationTable* tr1;
				SegmentationTable* tr2;
				SegmentationTable* ta;
				
				for(std::set<SegmentationTable*>::iterator ti = tables_to_merge.begin(), te = tables_to_merge.end(); ti != te; ti++)
				{
					for(std::set<SegmentationTable*>::iterator tii = tables_to_merge.begin(), tee = tables_to_merge.end(); tii != tee; tii++)
					{	
						if( ((*tii) != (*ti)) && ((*tii)->base == (*ti)->base) )
						{
							tr1 = *tii;
							tr2 = *ti;
							ta = UnionMerge(tr1, tr2);
							m = true;
							break;	
						}
					}
					if(m)
						break;
					
				}
					if(m)
					{
						tables_to_merge.erase(tr1);
						tables_to_merge.erase(tr2);
						delete (tr1);
						delete (tr2);
						tables_to_merge.insert(ta);
					}
			}while(m);
			/**/
			
//																														/**///testing convergence
//																														for(std::set<SegmentationTable*>::iterator ti = tables_to_merge.begin(), te = tables_to_merge.end(); ti != te; ti++)
//																														{
//																															SegmentationTable* table = *ti;
//																															//table->PrintPointers();
//																															if(table->aloc_base) errs() << "1 ";
//																															else errs() << "0 ";
//																														}
//																														errs() << "\n";
//																														/**/
			
			if(stop) break;
		}// end of natural merges
		
//																														/**///testing convergence
//																														for(std::set<SegmentationTable*>::iterator ti = tables_to_merge.begin(), te = tables_to_merge.end(); ti != te; ti++)
//																														{
//																															SegmentationTable* table = *ti;
//																															//table->PrintPointers();
//																															if(table->aloc_base) errs() << "1 ";
//																															else errs() << "0 ";
//																														}
//																														errs() << " - FINAL\n";
//																														/**/
																														
		//UnionMerge all remaining tables
		std::set<SegmentationTable*>::iterator ti = tables_to_merge.begin();
		std::set<SegmentationTable*>::iterator te = tables_to_merge.end();
		master_tables[i->first] = NULL;
		if(ti != te)
		{
			master_tables[i->first] = *ti;
			ti++;
			for(; ti != te; ti++)
				master_tables[i->first] = UnionMerge(master_tables[i->first], *ti);
			}
					
	}//end of alias sets
	

	errs() << "END OF CONVERGENCE\n"; 
	//
	//calculate remaining loose pointers and adding them to the master tables
	//
	
	/**/
	//for each alias set
	for(llvm::DenseMap<int, std::set<Value*> >::iterator i = vs.begin(), e = vs.end(); i != e; i++)
	{
		//for each value in it
		for(std::set<Value*>::iterator ii = i->second.begin(), ee = i->second.end(); ii != ee; ii++)
		{
			Value* v = *ii;
			//if there's no master table
			if( (master_tables[i->first] == NULL) )
			{			
				//if the pointer is not on loose_pointers already
				if(loose_pointers[i->first].find(v) == loose_pointers[i->first].end())
				{				
					//add value to loose_pointers
					loose_pointers[i->first].insert(v);
				}
			}
			else 
			{
				//if it is not present in the master table and also not already present in loose_pointers
				if( (RangedPointer::FindByValue(v, master_tables[i->first]->ranged_pointers) == NULL) && (loose_pointers[i->first].find(v) == loose_pointers[i->first].end()) )
				{			
					//add value to loose_pointers
					loose_pointers[i->first].insert(v);
				}/**/
			}
		}
	}	
	/**/		
	
																														/** ///testing loose pointers
																														errs() << "\nLOOSE POINTERS\n-------------------\n";
																														for(llvm::DenseMap<int, std::set<Value*> >::iterator i = loose_pointers.begin(), e = loose_pointers.end(); i != e; i++)
																														{
																															errs() << "Loose pointers of set " << (i->first) << " :\n";
																															for(std::set<Value*>::iterator ii = i->second.begin(), ee = i->second.end(); ii != ee; ii++)
																																errs() << **ii << "\n";
																															errs() << "\n";
																														}
																														/**/
																														
	//for each alias set
	for(llvm::DenseMap<int, std::set<Value*> >::iterator i = vs.begin(), e = vs.end(); i != e; i++)
	{
		//if there is no master table yet, build it with the loose_pointers
		if(master_tables[i->first] == NULL)
		{
			master_tables[i->first] = new SegmentationTable();
			//for each loose pointer, add it as a ranged pointer to the master table
			for(std::set<Value*>::iterator ii = loose_pointers[i->first].begin(), ee = loose_pointers[i->first].end(); ii != ee; ii++)
				master_tables[i->first]->ranged_pointers.insert( new RangedPointer((*ii), Min, Max, NULL, NULL) );
			//building master table
			BuildTable(master_tables[i->first]);
	
			//passing associations
			master_tables[i->first]->aloc_base = false;
			master_tables[i->first]->AliasSet = &(i->second);
			master_tables[i->first]->full_alias_set = true;
			master_tables[i->first]->has_no_alias = false;
		}
		else
		{
			//for each loose pointer
			for(std::set<Value*>::iterator ii = loose_pointers[i->first].begin(), ee = loose_pointers[i->first].end(); ii != ee; ii++)
			{
				//add it as a ranged pointer to the master table
				master_tables[i->first]->ranged_pointers.insert( new RangedPointer((*ii), Min, Max, NULL, NULL) );		
				//for each row in the master table
				for(set<SegmentationTableRow*>::iterator iii = master_tables[i->first]->rows.begin(), eee = master_tables[i->first]->rows.end(); iii != eee; iii++)
				{
					//add it to row
					(*iii)->pointers.insert((*ii));		
     			master_tables[i->first]->pointer_to_rows[(*ii)].insert((*iii));
				}
			}
			master_tables[i->first]->full_alias_set = true;
		}
	
		
																														/** ///testing alias set master table
																														errs() << "MASTER TABLE OF SET " << i->first << "\n-------------------\n";
																														master_tables[i->first]->PrintPointers();
																														/**/
		
	}
	
	//
	//check stores to predict memory range content 
	errs() << "END OF MASTER TABLES CALCULATION AND START OF CONTENT CALCULATION\n";
	//
	
	//for each alias set
	for(llvm::DenseMap<int, std::set<Value*> >::iterator i = vs.begin(), e = vs.end(); i != e; i++)
	{
		//for each row in it's master table
		for(std::set<SegmentationTableRow*>::iterator ii = master_tables[i->first]->rows.begin(), ee = master_tables[i->first]->rows.end(); ii != ee; ii++)
		{
			//make content Unknown
			(*ii)->content.setUnknown();
		}
		
		//for each pointer in it's master table
		for(llvm::DenseMap<Value*, std::set<SegmentationTableRow*> >::iterator ii = master_tables[i->first]->pointer_to_rows.begin(), ee = master_tables[i->first]->pointer_to_rows.end(); ii != ee; ii++)
		{
			Value* pointer = ii->first;
			//for each of its Users
			for(llvm::Value::use_iterator ui = pointer->use_begin(), ue = pointer->use_end(); ui != ue; ui++)
			{	
				User* u = *ui;
				//if user is a store instruction and pointer is used as pointer
				if(isa<StoreInst>(*u))
					if( ((StoreInst *)u)->getPointerOperand() == pointer )
					{
						Value* stored = ((StoreInst *)u)->getValueOperand();
						Range to_content;
						if(isa<ConstantInt>(*stored))
        		{
        			to_content = Range( ((ConstantInt*)stored)->getValue(), ((ConstantInt*)stored)->getValue() );
        		}
        		else
        		{
        			to_content = ra.getRange(stored);
        		}
        		
        		//for each row the pointers appears in
						for(std::set<SegmentationTableRow*>::iterator ri = ii->second.begin(), re = ii->second.end(); ri != re; ri++)
						{ 
							//arrage bitwidth problems
							int bitwidth = (*ri)->content.getLower().getBitWidth();
							to_content.setLower( to_content.getLower().sextOrTrunc(bitwidth) );
							    bitwidth = (*ri)->content.getUpper().getBitWidth();
							to_content.setUpper( to_content.getUpper().sextOrTrunc(bitwidth) );
							
							//phi the storing value's range with row's content
							(*ri)->content = (*ri)->content.unionWith(to_content);
						}
					}
			}
		}			

																														/**///testing alias set master table's content
																														//errs() << "CONTENTS OF MASTER TABLE OF SET " << i->first << "\n-------------------\n";
																														//master_tables[i->first]->PrintContent();	
																														/**/
		
	}
	/**/
	//
	// Filling statistics
	//
	
	NSegments = 0;
	NDefined = 0;
	NAvgSegments = 0;
	int nt = 0;
	NMaxSegments = 0;
	
	for(llvm::DenseMap<int, SegmentationTable* >::iterator i = master_tables.begin(), e = master_tables.end(); i != e; i++)
	{
		int nms = 0;
		SegmentationTable* table = i->second;
		nt++;
		for(std::set<SegmentationTableRow*>::iterator ri = table->rows.begin(), re = table->rows.end(); ri != re; ri++)
		{
			SegmentationTableRow* r  = *ri;
			if(!r->pointers.empty())
			{
				NSegments++;
				nms++;
				if( (r->content.getLower().sgt(Min)) or (r->content.getUpper().slt(Max)) )
					NDefined++;
			}
		}
		if(nms > NMaxSegments)
			NMaxSegments = nms;
	}
	if(nt > 0)
		NAvgSegments = NSegments/nt;
	
	return false;
}

SegmentationTable* AliasSetSegmentation::getTable(int AliasSetID)
{
	return master_tables[AliasSetID];
}

char AliasSetSegmentation::ID = 0;
static RegisterPass<AliasSetSegmentation> X("as-segmentation",
"Segmentation of Alias Sets", false, false);
/* namespace llvm */






/*//testing merges
	SegmentationTable* t[stm.size()];
	int c = 0;
	for(DenseMap<Value*, SegmentationTable*>::iterator i = stm.begin(), e = stm.end(); i != e; i++)
	{
		t[c] = i->second;
		c++;
	}
	
	APInt Vinte = Zero + 20;
	SegmentationTable* r = UnionMerge(t[1], t[0]);		
		errs() << "TEST\n";
		errs() << "Table: " << *(r->base) << "\n";
  	for(set<SegmentationTableRow*>::iterator ii = r->rows.begin(),
  	ee = r->rows.end(); ii != ee; ii++)
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
  	errs() << "\n";*/
