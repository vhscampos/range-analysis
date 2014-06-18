/*
 * TcProfilerLinkedLibrary.cpp
 *
 *  Created on: Dec 16, 2013
 *      Author: raphael
 */

#define __STDC_FORMAT_MACROS

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <map>
#include <set>
#include <iostream>

using namespace std;


extern "C"{

	void initLoopList();
	void collectLoopData(int64_t LoopHeaderBBPointer, int64_t tripCount, int64_t estimatedTripCount, int LoopClass);
	void flushLoopStats(char* moduleIdentifier);

}


/*
 * Here we implement a tree of tuples <int,int>
 *
 * This list works like a std::map<int,int>
 *
 */
typedef std::map<int64_t, int> LoopResults;

void printInOrder(LoopResults results, FILE* outStream, int LoopClass, char* moduleIdentifier, int64_t ID){

	for(LoopResults::iterator it = results.begin(); it != results.end(); it++){

		fprintf(outStream, "TripCount %d %s.%" PRId64 " %" PRId64 " %d\n",
					LoopClass,
					moduleIdentifier,
					ID,
					it->first,
					it->second);

	}

}

/*
 * Here we implement a linked list of loop statistics
 *
 * _loopStats is the type that makes possible to do the chain of elements
 */
class LoopStats {

public:

	LoopStats(){ LoopClass = 0; };
	virtual ~LoopStats(){};


	int LoopClass;
	LoopResults T;

};

void addInstance(LoopStats &Stats, int64_t tripCount, int64_t estimatedTripCount ){

	int64_t GroupID;

	if (estimatedTripCount >= tripCount-1 && estimatedTripCount <= tripCount-1)
			GroupID = 3;
	else if (estimatedTripCount <= sqrt((double)tripCount))
		GroupID = 0;
	else if (estimatedTripCount <= tripCount/2)
		GroupID = 1;
	else if (estimatedTripCount <= tripCount-2)
		GroupID = 2;
	else if (estimatedTripCount <= tripCount*2)
		GroupID = 4;
	else if (estimatedTripCount <= tripCount*tripCount)
		GroupID = 5;
	else
		GroupID = 6;

	if(Stats.T.count(GroupID)){
		Stats.T[GroupID]++;
	} else {
		Stats.T[GroupID] = 1;
	}

}

typedef std::map<int64_t, LoopStats> LoopList;


LoopList loops;

void initLoopList(){
}

void collectLoopData(int64_t LoopHeaderBBPointer, int64_t tripCount, int64_t estimatedTripCount, int LoopClass){

//	cerr << "Collecting loop data: Loop Header:" << LoopHeaderBBPointer
//			<< "Actual Trip count: " << tripCount
//			<< "Estimated Trip count: " << estimatedTripCount << "\n";

	if (!loops.count(LoopHeaderBBPointer)) loops[LoopHeaderBBPointer].LoopClass = LoopClass;

	//cerr << "Vai Adicionar Instância\n";

	addInstance(loops[LoopHeaderBBPointer], tripCount, estimatedTripCount);
}

void flushLoopStats(char* moduleIdentifier){

	FILE* outStream;
	outStream = fopen("loops.out", "a");

	if(!outStream){
		fprintf(stderr, "Error opening file loops.out\n");
	}else{

		for (LoopList::iterator it = loops.begin(); it != loops.end(); it++){
			printInOrder(it->second.T, outStream, it->second.LoopClass, moduleIdentifier, it->first);
		}

		fclose(outStream);
	}



}
