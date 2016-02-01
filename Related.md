# Related Works #

  * Widening with Thresholds for Programs with Complex Control Graphs, _Lies Lakhdar-Chaouch, Bertrand Jeannet, Alain GiraultJames Newsome_, ATVA 2011
    * **Summary**: the paper describes a better way to infer thresholds. Thresholds are constraints that one can use to do more precise widening. The method probably does not scale well. The experiments use very small benchmarks.
    * **What can we take from it?** The examples.

  * A Static Analyzer for Large Safety-Critical Software, _Bruno Blanchet, Patrick Cousot, Radhia Cousot, Jérôme Feret, Laurent Mauborgne, Antoine Miné, David Monniaux, Xavier Rival_, PLDI 2003
    * **Summary**: the paper describes a general approach (a bit on the Software Engineering side) to develop analyzers to real-world programs.
    * **What I can take from it?**: the many strategies to scale up the analysis.