from DataStructures import *

def crop_meet(op):
  """This is the meet operator of the cropping analysis. Whereas the growth
     analysis expands the bounds of each variable, regardless of intersections
     in the constraint graph, the cropping analysis shyrinks these bounds back
     to ranges that respect the intersections. Notice that we need to have
     a reference to the original state of the variable, e.g., 0, +, - and ?.
     We cannot store this information in the interval itself, because this
     interval is likely to change from, say, +, to a constant."""
  new_int = op.eval()
  has_changed = False
  if (op.sink.state == '-' or op.sink.state == '?') and gt(new_int.l, op.sink.interval.l):
    op.sink.interval.l = new_int.l
    has_changed = True
  if (op.sink.state == '+' or op.sink.state == '?') and lt(new_int.u, op.sink.interval.u):
    op.sink.interval.u = new_int.u
    has_changed = True
  return has_changed

def crop_dfs(use_map, active_ops, visited):
  """This method performs a depth-first search on the constraint graph, always
     applying the cropping operator on the nodes that it finds."""
  if len(active_ops) > 0:
    op = active_ops.pop()
    if not op.sink.name in visited:
      crop_meet(op)
      visited.add(op.sink.name)
      for next_use in use_map[op.sink.name]:
        active_ops.add(next_use)
    crop_dfs(use_map, active_ops, visited)

def growth_meet(op):
  """This is the meet operator of the growth analysis. The growth analysis
     will change the bounds of each variable, if necessary. Initially, each
     variable is bound to either the undefined interval, e.g. [., .], or to
     a constant interval, e.g., [3, 15]. After this analysis runs, there will
     be no undefined interval. Each variable will be either bound to a
     constant interval, or to [-, c], or to [c, +], or to [-, +]."""
  old_int = op.sink.interval
  new_int = op.eval()
  if isinstance(old_int, BottomInterval):
    op.sink.interval = new_int
  elif lt(new_int.l, old_int.l) and gt(new_int.u, old_int.u):
    op.sink.interval = Interval()
  elif lt(new_int.l, old_int.l):
    op.sink.interval = Interval('-', old_int.u)
  elif gt(new_int.u, old_int.u):
    op.sink.interval = Interval(old_int.l, '+')
  return old_int != op.sink.interval

def iterate_till_fix_point(use_map, active_vars):
  """This method implements the fix point iterator of the growth analysis."""
  if len(active_vars) > 0:
    next_variable = active_vars.pop()
    use_list = use_map[next_variable]
    for op in use_list:
      if growth_meet(op):
        active_vars.add(op.sink.name)
    iterate_till_fix_point(use_map, active_vars)

def store_abstract_states(variables):
  """This method stores the abstract state of each variable. The possible
     states are '0', '+', '-' and '?'. This method must be called before we
     run the cropping analysis, because these states will guide the meet
     operator when cropping ranges."""
  for var in variables:
    if var.interval.l == '-' and var.interval.u == '+':
      var.state = '?'
    elif var.interval.l == '-':
      var.state = '-'
    elif var.interval.u == '+':
      var.state = '+'
    else:
      var.state = '0'

def int_op(Operations):
  """This method finds which operations contain non-trivial intersection.
     The variables used in these operations will be the starting point of the
     cropping analysis, and shall be returned by this method."""
  non_trivial_set = set()
  for op in Operations:
    if isinstance(op, UnaryOp) and (op.i.u != '+' or op.i.l != '-'):
      non_trivial_set.add(op)
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
#    printInstance(component, CompDefMap.values(), CompUseMap, EntryPoints)
    iterate_till_fix_point(CompUseMap, set(EntryPoints))
    for var in component:
      if var in SymbMap.keys():
        for op in SymbMap[var]:
          op.fixIntersects()
    store_abstract_states(component)
    for op in int_op(CompDefMap.values()):
      crop_dfs(CompUseMap, set([op]), set())
    propagateToNextSCC(component, UseMap)
  toDot("Final graph", Variables, Operations)

# readGraph(findIntervals)
processGraph("g9.dat", findIntervals)
