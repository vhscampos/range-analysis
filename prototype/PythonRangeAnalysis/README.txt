This folder contains a suite of algorithms and data structures to solve the
range analysis problem. I have coded two algorithms:

(i) Cousot.py, which uses the widening and narrowing operators of Cousot
and Cousot.

(ii) CropDFS.py, which performs a depth first search on the constraint
graph, cropping the intervals after intersections.

There is a small suite of input graphs in the graphs folder. Each input
graph is in python source code. To run a file, just type:

$> python Cousot

and enter the name of an input file, e.g.: graphs/g19.dat. Each of these
graphs has its dot and pdf versions in the dot folder.

The bck/ folder contains a number of previous attempts to code the
algorithm. I think I will not go back to those programs, now that I have an
almost definitive prototype of the range analysis solver.
