#include <list>
#include <set>
#include <map>
#include <queue>
#include <stack>
#include <sstream>
#include <iostream>

#include "PointerAnalysis.h"

//char PointerAnalysis::ID = 0;

const bool debug = false;
int teste = 1;

// ============================================= //

///  Default constructor
PointerAnalysis::PointerAnalysis()
{
    if (debug) std::cerr << "Initializing Pointer Analysis" << std::endl;
	numMerged = 0;
	numCallsRemove = 0;
}

// ============================================= //

/// Destructor
PointerAnalysis::~PointerAnalysis()
{
    if (debug) std::cerr << "Terminating Pointer Analysis" << std::endl;
}

// ============================================= //

/**
 * Add a constraint of type: A = &B
 */
void PointerAnalysis::addAddr(int A, int B)
{
    if (debug) std::cerr << "Adding Addr Constraint: " <<  A << " = &" << B << std::endl;

	// Ensure nodes A and B exists.
	addNode(A);
	addNode(B);

	// Add B to pts(A)
	addToPts(B, A);
}

// ============================================= //

/**
 * Add a constraint of type: A = B
 */
void PointerAnalysis::addBase(int A, int B)
{
    if (debug) std::cerr << "Adding Base Constraint: " << A << " = " << B << std::endl;

	// Ensure nodes A and B exists.
	addNode(A);
	addNode(B);

	// Add edge from B to A
	addEdge(B, A);
}

// ============================================= //

/**
 * Add a constraint of type: *A = B
 */
void PointerAnalysis::addStore(int A, int B)
{
    if (debug) std::cerr << "Adding Store Constraint: *" << A << " = " << B << std::endl;

	// Ensure nodes A and B exists.
	addNode(A);
	addNode(B);

	// Add the constraint
	stores[A].insert(B);
}

// ============================================= //

/**
 * Add a constraint of type: A = *B
 */
void PointerAnalysis::addLoad(int A, int B)
{
    if (debug) std::cerr << "Adding Load Constraint: " << A << " = *" << B << std::endl;

	// Ensure nodes A and B exists.
	addNode(A);
	addNode(B);

	// Add the constraint
	loads[B].insert(A);
}

// ============================================= //

/**
 * Return the set of positions pointed by A:
 *   pointsTo(A) = {B1, B2, ...}
 */
std::set<int> PointerAnalysis::pointsTo(int A)
{
    if (debug) std::cerr << "Recovering Points-to-set of "<< A << std::endl;

	int repA = vertices[A];
	return pointsToSet[repA];
}

// ============================================= //

/**
 * Add a new node to the graph if it doesn't already exist.
 */
void PointerAnalysis::addNode(int id)
{
    if (debug) std::cerr << "Adding Node " << id << std::endl;

	// Only add the node if it doesn't exist
	if (vertices.find(id) == vertices.end()) 
	{
		// Its current representative is itself and it is active
		vertices[id] = id;
		activeVertices.insert(id);
	}
}

// ============================================= //

/**
 * Add an edge in the graph.
 */
void PointerAnalysis::addEdge(int fromId, int toId)
{
    if (debug) std::cerr << "Adding Edge from "<< fromId << " to " << toId << std::endl;

	// We work with the representatives, so get them first.
	int repFrom = fromId;//vertices[fromId];
	int repTo = toId;//vertices[toId];

	// Add the edge (both directions)
	from[repFrom].insert(repTo);
    to[repTo].insert(repFrom);
}

// ============================================= //

/**
 * Add a node to the points-to set of another node
 */
void PointerAnalysis::addToPts(int pointed, int pointee)
{
    if (debug) std::cerr << "Adding " << pointed << " to pts(" << pointee << ")"  << std::endl;

	// Add the reference
	pointsToSet[pointee].insert(pointed);
}

// ============================================= //

void PointerAnalysis::cycleSearch(int source, int target) {

	numCallsRemove++;

	if (debug) std::cerr << "Looking for path between " << source << " and " << target << std::endl;

	// Tracking vars
	bool cycleFound = false;
	std::map<int,int> origin;
	//IntMap origin;
	std::set<int> visited;
	//IntSet visited;
	std::list<int> cycle;
	std::stack<int> queue;

	// Start with source on the queue
	if (debug) std::cerr << "Starting with " << source << " on the queue" << std::endl;;
	queue.push(source);
	origin[source] = target;
	visited.insert(source);

	// Do a Breadth-First Search
	while ( !queue.empty() ) {
		// Pop next vertex
		int current = queue.top();
		queue.pop();
		if (debug) std::cerr << "Popped " << current << " from the queue" << std::endl;;

		// Found a cycle!
		if (current == target) {
			if (debug) std::cerr << "Reached Target!!" << std::endl;
			cycleFound = true;
			break;
		}

		// Loop through the neighbours
		IntSet::iterator n;
		for (n = from[current].begin(); n != from[current].end(); ++n) {

			// Get the representative
			int repN = vertices[*n];


			// Add it to the search queue, if not there yet
			if (visited.find(repN) == visited.end()) {
				if (debug) std::cerr << "Add " << repN << " to the queue and marking it visited" << std::endl;
				queue.push(repN);
				visited.insert(repN);

				// Mark 'current' as the antecessor of 'n'
				if (debug) std::cerr << "Mark " << current << " as antecessor of " << *n << std::endl;
				origin[repN] = current;
			}
		}
	}

	// If we found a cycle, build a list with it

	if (cycleFound) {
		int current = target;
		while (current != source) {
			cycle.push_front(current);
			current = origin[current];
		}
		cycle.push_front(source);

		int a = cycle.front();
		cycle.pop_front();
		while (!cycle.empty()) {
			int b = cycle.front();
			cycle.pop_front();
			merge(a, b);
			a = b;
		}
	}

}

// ============================================= //

void PointerAnalysis::removeCycles()
{
    if (debug) std::cerr << "Running cycle removal algorithm" << std::endl;

	numCallsRemove++;

    // Some needed variables
    IntMap order;
    IntMap repr;
    int i = 0;
    IntSet current;
    IntDeque S;

    // At first, no node is visited and their representatives are themselves
    if (debug) std::cerr << ">> Initializing structures" << std::endl;
    IntSet::iterator V;
    for (V = activeVertices.begin(); V != activeVertices.end(); V++)
	{
        order[*V] = 0;
        repr[*V] = *V;
    }

    // Visit all unvisited nodes
    if (debug) std::cerr << ">> Visiting vertices" << std::endl;
    for (V = activeVertices.begin(); V != activeVertices.end(); V++)
	{
        if (order[*V] == 0)
		{
            visit(*V, order, repr, i, current, S);
        }
    }

    // Merge those whose representatives are not themselves
    if (debug) std::cerr << ">> Merging vertices" << std::endl;
    for (V = activeVertices.begin(); V != activeVertices.end(); )
	{
        IntSet::iterator nextIt = V;
        nextIt++;
        if (debug) std::cerr << " * Current V (Before): " << *V << std::endl;
        if (debug) std::cerr << " * Current nextIt (Before): " << *nextIt << std::endl;
        if (repr[*V] != *V) 
		{
            merge(*V, repr[*V]);
        }
        V = nextIt;
        if (debug) std::cerr << " * Current V (After): " << *V << std::endl;
        if (debug) std::cerr << " * Current nextIt (After): " << *nextIt << std::endl;
    }

    if (debug)
    {
        std::cerr << ">> Printing active vertices" << std::endl;
        for (V = activeVertices.begin(); V != activeVertices.end(); V++) 
        {
            std::cerr << *V << std::endl;
        }
    }

}

// ============================================= //

void PointerAnalysis::visit(int Node, IntMap& Order, IntMap& Repr,
	int& idxOrder, IntSet& Curr, IntDeque& Stack)
{
    if (debug) std::cerr << "  - Visiting vertex " << Node << std::endl;

	idxOrder++;
    Order[Node] = idxOrder;

    IntSet::iterator w;
    for (w = from[Node].begin(); w != from[Node].end(); w++)
	{
        if (Order[*w] == 0) visit(*w, Order, Repr, idxOrder, Curr, Stack);
        if (Curr.find(*w) == Curr.end())
		{
            Repr[Node] = (Order[Repr[Node]] < Order[Repr[*w]]) ?
                Repr[Node] :
                Repr[*w]
            ;
        }
    }

    if (Repr[Node] == Node)
	{
        Curr.insert(Node);
        while (!Stack.empty())
		{
            int w = Stack.front();
            if (Order[w] <= Order[Node]) break;
            else
			{
                Stack.pop_front();
                Curr.insert(w);
                Repr[w] = Node;
            }
        }
        // Push(TopologicalOrder, Node)
    }
    else
	{
        Stack.push_front(Node);
    }
}

// ============================================= //

/**
 * Merge two nodes.
 * @param id the noded being merged
 * @param target the noded to merge into
 */
void PointerAnalysis::merge(int id, int target)
{
    if (debug) std::cerr << " - Merging " << id << " into " << target << std::endl;

	// Remove all edges id->target, target->id
    from[id].erase(target);
    to[target].erase(id);
    from[target].erase(id);
    to[id].erase(target);

    // Move all edges id->v to target->v
    if (debug) std::cerr << "Outgoing edges..." << std::endl;
    IntSet::iterator v;
    for (v = from[id].begin(); v != from[id].end(); v++)
	{
        from[target].insert(*v);
        to[*v].erase(id);
        to[*v].insert(target);
    }

    // Move all edges v->id to v->target
    if (debug) std::cerr << "Incoming edges..." << std::endl;
    for (v = to[id].begin(); v != to[id].end(); v++)
	{
        to[target].insert(*v);
        from[*v].erase(id);
        from[*v].insert(target);
    }

    // Mark the representative vertex
    vertices[id] = target;
    if (debug) std::cerr << "Removing vertice " << id << " from active." << std::endl;
	activeVertices.erase(id);

    // Merge Stores
    if (debug) std::cerr << "Stores..." << std::endl;
    for (v = stores[id].begin(); v != stores[id].end(); v++)
	{
        stores[target].insert(*v);
    }
    stores[id].clear(); // Not really needed, I think

    // Merge Loads
    if (debug) std::cerr << "Loads..." << std::endl;
    for (v = loads[id].begin(); v != loads[id].end(); v++)
	{
        loads[target].insert(*v);
    }
    loads[id].clear();

    // Join Points-To set
    if (debug) std::cerr << "Points-to-set..." << std::endl;
    for (v = pointsToSet[id].begin(); v != pointsToSet[id].end(); v++)
	{
        pointsToSet[target].insert(*v);
    }
    if (debug) std::cerr << "End of merging..." << std::endl;

	// Count this merge
	numMerged++;
}

// ============================================= //

bool PointerAnalysis::comparePts(int a, int b) {

	if (pointsToSet[a].size() != pointsToSet[b].size())
		return false;

	IntSet::iterator V;
	for (V = pointsToSet[a].begin(); V != pointsToSet[a].end(); V++) 
	{
		if (pointsToSet[b].find(vertices[*V]) == pointsToSet[b].end()) 
			return false;
	}
	return true;
}

// ============================================= //

/**
 * Execute the pointer analysis
 * TODO: Add info about the analysis
 */
void PointerAnalysis::solve(bool withCycleRemoval)
{
	numMerged = 0;
	numCallsRemove = 0;
    std::set<std::string> R;
    IntSet WorkSet = activeVertices;
    IntSet NewWorkSet;

    if (debug) std::cerr << "Starting the analysis" << std::endl;

    while (!WorkSet.empty()) {

    	if (debug)  std::cerr << "WorkSet.size: " << WorkSet.size() << "\n";

        int Node = *WorkSet.begin();
        Node = vertices[Node];
        WorkSet.erase(WorkSet.begin());

        if (debug)
        {
            std::cerr << ">> New Step" << std::endl;
            std::cerr << " - Current Node: " << Node << std::endl;
        }

        // For V in pts(Node)
        IntSet::iterator V;
        for (V = pointsToSet[Node].begin(); V != pointsToSet[Node].end(); V++ )
		{
            int reprV = vertices[*V];
            if (debug)
            {
                std::cerr << "   - Current V: " << *V << std::endl;
                std::cerr << "   - Repr of V: " << reprV << std::endl;
                std::cerr << "   - Load Constraints" << std::endl;
            }
            // For every constraint A = *Node
            IntSet::iterator A;
            for (A=loads[Node].begin(); A != loads[Node].end(); A++) 
            {
                // If V->A not in Graph
                // Get the repr of A
                int reprA = vertices[*A];
                if (from[reprV].find(reprA) == from[reprV].end()) 
				{
                    addEdge(reprV, reprA);
                    NewWorkSet.insert(reprV);
                }
            }

            if (debug) std::cerr << "   - Store Constraints" << std::endl;
            // For every constraint *Node = B
            IntSet::iterator B;
            for (B=stores[Node].begin(); B != stores[Node].end(); B++) 
            {
                // If B->V not in Graph
                // Get the repr of B
                int reprB = vertices[*B];
                if (from[reprB].find(reprV) == from[reprB].end()) 
				{
                    addEdge(reprB, reprV);
                    NewWorkSet.insert(reprB);
                }
            }
        }

        if (debug) std::cerr << " - End step" << std::endl;
        // For Node->Z in Graph

        for  (IntSet::iterator Z = from[Node].begin(); Z != from[Node].end(); Z++ )
		{
            int ZVal = *Z;
			int repN = vertices[Node];
			int repZ = vertices[ZVal];
            std::stringstream sstm;
            sstm << repN << "->" << repZ;
            std::string edge = sstm.str();

            if (debug) std::cerr << " - Comparing pts of " << Node << " and " << ZVal << std::endl;

			if (withCycleRemoval) {
				// Compare points-to sets
				if (debug) {
					std::cerr << "    - Reps: " << repN << " and " << repZ << std::endl;
					std::cerr << "PTS("<<repN<<"): ";
					//for(int i : pointsToSet[repN]) std::cerr << i << " ";
					std::cerr << std::endl;
					std::cerr << "PTS("<<repZ<<"): ";
					//for(int i : pointsToSet[repZ]) std::cerr << i << " ";
					std::cerr << std::endl;
					std::cerr << "PTS == PTS ? " << (pointsToSet[repZ] == pointsToSet[repN]) << std::endl;
					std::cerr << "ComparePTS : " << comparePts(repZ, repN) << std::endl;
					std::cerr << "Edge: [" << edge << "] (" << (R.find(edge) == R.end()) << ")" << std::endl;
				}
				if ( repZ != repN && pointsToSet[repZ] == pointsToSet[repN]
						&& R.find(edge) == R.end() )
				{
					if (debug) std::cerr << " - Removing cycles..." << std::endl;
					//removeCycles();
					cycleSearch(repZ, repN);
					R.insert(edge);
					if (debug) std::cerr << " - Cycles removed" << std::endl;
				}
			}

            // Merge the points-To Set
            if (debug) std::cerr << " - Merging pts" << std::endl;
            bool changed = false;
            for (V = pointsToSet[Node].begin(); V != pointsToSet[Node].end(); V++)
			{
                changed |= pointsToSet[ZVal].insert(*V).second;
            }

            // Add Z to WorkSet if pointsToSet(Z) changed
            if (changed)
			{
                NewWorkSet.insert(ZVal);
            }

            if (debug) std::cerr << " - End of step" << std::endl;
        }

        // Swap WorkSets if needed
        if (WorkSet.empty()) WorkSet.swap(NewWorkSet);
    }

    // Consolidate Points-To Set
    IntMap::iterator NodeIt;
    for (NodeIt = vertices.begin(); NodeIt != vertices.end(); NodeIt++) {
        // Only have to consolidate if vertex is not active (was merged)
        // (in other words, when its repr. is not itself)
        if (NodeIt->first != NodeIt->second) {
            const IntSet& ptsR = pointsToSet[NodeIt->second];
            IntSet::iterator V;
            for (V = ptsR.begin(); V != ptsR.end(); V++) {
                pointsToSet[NodeIt->first].insert(*V);
            }
        }
    }
}

// ============================================= //

/// Prints the graph to std output
void PointerAnalysis::print()
{
    std::cout << "# of Vertices: ";
    std::cout << vertices.size() << std::endl;
    IntSet::iterator it;
    IntMap::iterator v;

    // Print Vertices Representatives
    std::cout << "Representatives: " << std::endl;
    for (v = vertices.begin(); v != vertices.end(); v++)
	{
        std::cout << v->first << " -> ";
        std::cout << v->second << std::endl;
    }
    std::cout << std::endl;

    // Print Vertices Connections
    std::cout << "Connections (Graph): " << std::endl;
    for (it = activeVertices.begin(); it != activeVertices.end(); it++)
	{
        std::cout << *it << " -> ";
        IntSet::iterator n;
        for (n = from[*it].begin(); n!= from[*it].end(); n++)
        {
            std::cout << *n << " ";
        }
        //std::cout << "0";
        std::cout << std::endl;
    }
    std::cout << std::endl;

    // Prints PointsTo sets
    std::cout << "Points-to-set: " << std::endl;
    for (v = vertices.begin(); v != vertices.end(); v++)
	{
        std::cout << v->first << " -> {";
        IntSet::iterator n;
        for (n = pointsToSet[v->first].begin(); n != pointsToSet[v->first].end();  n++)
        {
            std::cout << *n << ", ";
        }

        std::cout << "}";
        std::cout << std::endl;
    }
    std::cout << std::endl;

}

// ============================================= //

void PointerAnalysis::printDot(std::ostream& output, std::string graphName,
        std::map<int, std::string> names) {

    // Used to create the labels
    IntSetMap verticesLabels;

    // Iterators used when traversing the structures
    IntMap::iterator mapIt;
    IntSet::iterator setIt;
    IntSet::iterator setIt2;

    // For each active vertex, build a set with the merged vertices
    // represented by it
    for (mapIt = vertices.begin(); mapIt != vertices.end(); mapIt++) {
        // If the vertice is not merge, it is the one to be drawn
        if (mapIt->first == mapIt->second) {
            verticesLabels[mapIt->first].insert(mapIt->first);
        }
        // If it is merged, its name goes to the label of its representative
        else {
            verticesLabels[mapIt->second].insert(mapIt->first);
        }
    }

    // It is a directed graph
    output << "digraph \"" << graphName << "\" {" << std::endl;

    // Declare the vertices
    for (setIt = activeVertices.begin(); setIt != activeVertices.end(); setIt++) {

        // Default style
        std::string style = "color=blue,style=solid";

        // Declare the node by its ID
        output << "    " << *setIt << " [label=\"";

        // Compute the label and change the style if there are merged vertices
        setIt2 = verticesLabels[*setIt].begin();
        int n = *(setIt2++);
        if (names.find(n) != names.end())
            output << names[n];
        else
            output << "#" << n;
        for ( ; setIt2 != verticesLabels[*setIt].end(); setIt2++) {
            n = *setIt2;
            if (names.find(n) != names.end())
                output << ", " << names[*setIt2];
            else
                output << ", #" << n;
            style = "color=darkgreen,style=bold";
        }

        // Print out the style
        output << "\"," << style << "];" << std::endl;
    }

    // Print the Edges
    for (setIt = activeVertices.begin(); setIt != activeVertices.end(); setIt++) {
        for (setIt2 = from[*setIt].begin(); setIt2 != from[*setIt].end() ; setIt2++) {
            if (*setIt == *setIt2) continue;
            output << "    " << *setIt << " -> " << *setIt2 << ";" << std::endl;
        }
    }

    // Print Pointed memories
    for (setIt = activeVertices.begin(); setIt != activeVertices.end(); setIt++) {

        // Skip if current vertex doesn't point to someone
        if (pointsToSet[*setIt].size() == 0) continue;

        // Print the node with the pointed locations
        output << "    pts" << *setIt << " [label=\"";
        setIt2 = pointsToSet[*setIt].begin();
        int n = *(setIt2++);
        if (names.find(n) == names.end()) 
            output << "#" << n;
        else
            output << names[n];
        for (; setIt2 != pointsToSet[*setIt].end() ; setIt2++) {
            n = *setIt2;
            if (names.find(n) == names.end()) 
                output << ", #" << *setIt2;
            else
                output << ", " << names[*setIt2];
        }
        output << "\",color=red,style=dashed,shape=box];" << std::endl;

        // Bind the node to the vertex
        output << "    " << *setIt << " -> pts" << *setIt << " [color=red,style=dashed];" << std::endl;

    }

    output << "}" << std::endl;
}

// ============================================= //

/// Returns the points-to map
std::map<int, std::set<int> > PointerAnalysis::allPointsTo() {
    return pointsToSet;
}

// ============================================= //

// Returns the amount of vertices that were merged
int PointerAnalysis::getNumOfMertgedVertices() {
	return numMerged;
}

// ============================================= //

int PointerAnalysis::getNumCallsRemove() {
	return numCallsRemove;
}

// ============================================= //

int PointerAnalysis::getNumVertices() {
	return this->vertices.size();
}

// ============================================= //
