from DataStructures import *

def widen_meet(op):
  """This is the meet operator of the growth analysis. The growth analysis
     will change the bounds of each variable, if necessary. Initially, each
     variable is bound to either the undefined interval, e.g. [., .], or to
     a constant interval, e.g., [3, 15]. After this analysis runs, there will
     be no undefined interval. Each variable will be either bound to a
     constant interval, or to [-, c], or to [c, +], or to [-, +]."""
  ij = op.sink.interval
  kl = op.eval()
  if isinstance(ij, BottomInterval):
    op.sink.interval = kl
  elif lt(kl.l, ij.l) and gt(kl.u, ij.u):
    op.sink.interval = Interval()
  elif lt(kl.l, ij.l):
    op.sink.interval = Interval('-', ij.u)
  elif gt(kl.u, ij.u):
    op.sink.interval = Interval(ij.l, '+')
  return ij != op.sink.interval

def narrow_meet(op):
  """This is the meet operator of the cropping analysis. Whereas the growth
     analysis expands the bounds of each variable, regardless of intersections
     in the constraint graph, the cropping analysis shyrinks these bounds back
     to ranges that respect the intersections."""
  has_changed = False
  ij = op.sink.interval
  kl = op.eval()
  if ij.l == '-' and kl.l != '-':
    ij.l = kl.l
    has_changed = True
  elif ij.l != min(ij.l, kl.l):
    ij.l = min(ij.l, kl.l)
    has_changed = True
  if ij.u == '+' and kl.u != '+':
    ij.u = kl.u
    has_changed = True
  elif ij.u != max(ij.u, kl.u):
    ij.u = max(ij.u, kl.u)
    has_changed = True
  return has_changed

def iterate_till_fix_point(use_map, active_vars, meet):
  """This method finds the growth pattern of each variable. If we pass to it
     the widen_meet, then we will have the growth analysis. If we pass the
     narrow_meet, then we will have the cropping analysis."""
  if len(active_vars) > 0:
    next_variable = active_vars.pop()
    use_list = use_map[next_variable]
    for op in use_list:
      if meet(op):
        active_vars.add(op.sink.name)
    iterate_till_fix_point(use_map, active_vars, meet)

def int_op(Variables, UseMap):
  """This method finds which operations contain non-trivial intersection.
     The variables used in these operations will be the starting point of the
     cropping analysis, and shall be returned by this method."""
  non_trivial_set = set()
  for var_name in Variables:
    for op in UseMap[var_name]:
      if isinstance(op, UnaryOp) and (op.i.u != '+' or op.i.l != '-'):
        non_trivial_set.add(var_name)
        break
  return non_trivial_set

def propagateToNextSCC(component, UseMap):
  """This method evaluates once each operation that uses a variable in
     component, so that the next SCCs after component will have entry
     points to kick start the range analysis algorithm."""
  for var in component:
    for op in UseMap[var.name]:
      ij = op.sink.interval
      kl = op.eval()
      if isinstance(ij, BottomInterval):
        op.sink.interval = kl
    
def findIntervals(Variables, Operations):
  """Finds the intervals of a SCC."""
  UseMap = buildUseMap(Variables.values(), Operations)
  SymbMap = buildSymbolicIntersectMap(Operations)
  sccList = Nuutila(Variables, UseMap, SymbMap)
  for component in sccList:
    CompDefMap = buildDefMap([op for op in Operations if op.sink in component])
    CompUseMap = buildUseMap(component, CompDefMap.values())
    EntryPoints = [var.name for var in component if not isinstance(var.interval, BottomInterval)]
# printInstance(component, CompDefMap.values(), CompUseMap, EntryPoints)
    iterate_till_fix_point(CompUseMap, set(EntryPoints), widen_meet)
    for var in component:
      if var in SymbMap.keys():
        for op in SymbMap[var]:
          op.fixIntersects()
    iterate_till_fix_point(CompUseMap, set([var.name for var in component]), narrow_meet)
    propagateToNextSCC(component, UseMap)
  toDot("After cropping analysis", Variables, Operations)

readGraph(findIntervals)
#processGraph("g9.dat", findIntervals)
