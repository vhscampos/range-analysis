class InvalidIntervalException(Exception):
  def __init__(self, l, u):
    self.l = l
    self.u = u
  def __str__(self):
    return "Invalid interval [" + str(l) + ", " + str(u) + "]"

class ArithmeticException(Exception):
  def __init__(self, msg):
    self.msg = msg
  def __str__(self):
    return self.msg

def lt(x, y):
  if x == y:
    return False
  elif x == '-' or y == '+':
    return True
  elif y == '-' or x == '+':
    return False
  else:
    return x < y

def leq(x, y):
  if x == y or x == '-' or y == '+':
    return True
  elif y == '-' or x == '+':
    return False
  else:
    return x < y

def gt(x, y):
  if x == y:
    return False
  elif x == '+' or y == '-':
    return True
  elif y == '+' or x == '-':
    return False
  else:
    return x > y

def geq(x, y):
  if x == y or x == '+' or y == '-':
    return True
  elif y == '+' or x == '-':
    return False
  else:
    return x > y

def min(x, y):
  if lt(x, y):
    return x
  else:
    return y

def max(x, y):
  if gt(x, y):
    return x
  else:
    return y

def add(x, y):
  if (x == '-' and y == '+') or (y == '-' and x == '+'):
    raise ArithmeticException("Adding minus and plus infinity.")
  elif x == '-' or y == '-':
    return '-'
  elif x == '+' or y == '+':
    return '+'
  else:
    return x + y

def mul(x, y):
  if (x == 0 or y == '0'):
    return 0
  elif (x == '-' and lt(y, 0)) or (y == '-' and lt(x, 0)):
    return '+'
  elif (x == '+' and gt(y, 0)) or (y == '+' and gt(x, 0)):
    return '+'
  elif (x == '-' and gt(y, 0)) or (y == '-' and gt(x, 0)):
    return '-'
  elif (x == '+' and lt(y, 0)) or (y == '+' and lt(x, 0)):
    return '-'
  else:
    return x * y

def addItv(i1, i2):
  return Interval(add(i1.l, i2.l), add(i1.u, i2.u))

class Interval:
  def __init__(self, l='-', u='+'):
    if (l != '-' and u != '+'):
      if (l > u):
        raise InvalidIntervalException(l, u)
    self.l = l;
    self.u = u;
  def intersection(self, i):
    if (gt(self.l, i.u) or lt(self.u, i.l)):
      return BottomInterval()
    else:
      return Interval(max(self.l, i.l), min(self.u, i.u))
  def union(self, i):
    return Interval(min(self.l, i.l), max(self.u, i.u))
  def __str__(self):
    return "[" + str(self.l) + ", " + str(self.u) + "]"

class SymbolicInterval(Interval):
  """This is an interval that contains a symbolic limit, which is given by
     the bounds of a program name, e.g.: [-inf, ub(b) + 1]"""
  def __init__(self, bound, op='=='):
    self.bound = bound
    self.op = op
    self.l = '-'
    self.u = '+'
  def __str__(self):
    if self.op == '==':
      return "[lb(" + self.bound.name + "), ub(" + self.bound.name + ")]"
    elif self.op == '<=':
      return "[-, ub(" + self.bound.name + ")]"
    elif self.op == '<':
      return "[-, ub(" + self.bound.name + ") - 1]"
    elif self.op == '>=':
      return "[lb(" + self.bound.name + "), +]"
    elif self.op == '>':
      return "[lb(" + self.bound.name + ") + 1, +]"
    else:
      return "[" + str(self.l) + ", " + str(self.u) + "]"

class BottomInterval(Interval):
  """This interval is used to represent the empty interval. It arises, for
     instance, from the intersection of disjoint intervals."""
  def __str__(self):
    return "[., .]"

class VarNode:
  """A VarNode represents a program variable."""
  def __init__(self, name, interval=Interval()):
    self.name = name
    self.interval = interval
  def __str__(self):
    return self.name + str(self.interval)

class UnaryOp:
  """A constraint like sink = a * source + b \intersec [l, u]"""
  def __init__(self, source, sink, a=1, b=0, interval=Interval('-', '+')):
    self.source = source
    self.sink = sink
    self.a = a
    self.b = b
    self.i = interval
  def __str__(self):
    self_str = str(self.sink) + " = " + str(self.a) + "* " + str(self.source)
    self_str += str(self.b) + " \int " + str(self.i)
    return self_str
  def eval(self):
    """Read the interval in source, apply the operation on it, and return it."""
    l = add(mul(self.a, self.source.interval.l), self.b)
    u = add(mul(self.a, self.source.interval.u), self.b)
    auxInterval = Interval(l, u)
    return auxInterval.intersection(self.i)
  def fixIntersects(self):
    """Replace symbolic intervals with hard-wired constants."""
    if isinstance(self.i, SymbolicInterval):
      l = self.i.bound.interval.l
      u = self.i.bound.interval.u
      if self.i.op == '==':
        self.i = Interval(l, u)
      elif self.i.op == '<=':
        self.i = Interval(self.i.l, u)
      elif self.i.op == '<':
        self.i = Interval(self.i.l, u - 1)
      elif self.i.op == '>=':
        self.i = Interval(l, self.i.u)
      elif self.i.op == '>':
        self.i = Interval(l + 1, self.i.u)
      else:
        self.i = Interval()
  def toDotStr(self):
    lb = "  " + str(hash(self)) + " [label =\""
    space = ""
    if self.a != 1:
      lb += "*(" + str(self.a) + ")"
      space = " "
    if self.b != 0:
      lb += space + "+(" + str(self.b) + ")"
      space = " "
    if isinstance(self.i, SymbolicInterval) or self.i.l != '-' or self.i.u != '+':
      lb += space + "INT" + str(self.i)
    lb += "\"]\n"
    lb += "  " + self.source.name + " -> " + str(hash(self)) + "\n"
    lb += "  " + str(hash(self)) + " -> " + self.sink.name + "\n"
    return lb

class PlusOp:
  """A constraint like sink = a * source + b \intersec [l, u]"""
  def __init__(self, src1, src2, sink):
    self.src1 = src1
    self.src2 = src2
    self.sink = sink
  def __str__(self):
    return self.sink.name + " = " + self.src1.name + " + " + self.src2.name
  def eval(self):
    """Read the interval in source, apply the operation on it, and return it."""
    int1 = self.src1.interval
    int2 = self.src2.interval
    intSk = addItv(int1, int2)
    return intSk
  def fixIntersects(self):
    """Replace symbolic intervals with hard-wired constants. Normally this
       kind of operations have no intersect to fix, but the method is here so
       that we can invoke it on any kind of operation."""
  def toDotStr(self):
    lb = "  " + str(hash(self)) + " [label =\" + \"]\n"
    lb += "  " + self.src1.name + " -> " + str(hash(self)) + "\n"
    lb += "  " + self.src2.name + " -> " + str(hash(self)) + "\n"
    lb += "  " + str(hash(self)) + " -> " + self.sink.name + "\n"
    return lb

# Graph 0:
Variables0 = {
  'k' : VarNode('k', Interval(0, 0)),
  'k0': VarNode('k0', Interval(100, 100)),
  'k1': VarNode('k1', BottomInterval()),
  'k2': VarNode('k2', BottomInterval())
}
Operations0 = [
  PlusOp(Variables0['k0'], Variables0['k'], Variables0['k1']),
  UnaryOp(Variables0['k1'], Variables0['k2'], b=1),
  UnaryOp(Variables0['k2'], Variables0['k'])
]
UseMap0 = {
  'k' : [Operations0[0]],
  'k0': [Operations0[0]],
  'k1': [Operations0[1]],
  'k2': [Operations0[2]]
}

# Graph 1:
Variables1 = {
  'k0': VarNode('k0', Interval(3, 3)),
  'k1': VarNode('k1', Interval(5, 5)),
  'k2': VarNode('k2', BottomInterval()),
  'k3': VarNode('k3', Interval(7, 7)),
  'k4': VarNode('k4', BottomInterval())
}
Operations1 = [
  PlusOp(Variables1['k0'], Variables1['k1'], Variables1['k2']),
  PlusOp(Variables1['k1'], Variables1['k3'], Variables1['k4']),
  PlusOp(Variables1['k2'], Variables1['k4'], Variables1['k3'])
]
UseMap1 = {
  'k0': [Operations1[0]],
  'k1': [Operations1[0], Operations1[1]],
  'k2': [Operations1[2]],
  'k3': [Operations1[1]],
  'k4': [Operations1[2]]
}

# Graph 2:
Variables2 = {
  'i': VarNode('i', Interval(0, 0)),
  'i1': VarNode('i1', BottomInterval()),
  'i2': VarNode('i2', BottomInterval()),
  'j': VarNode('j', Interval(100, 100)),
  'j1': VarNode('j1', BottomInterval()),
  'j2': VarNode('j2', BottomInterval()),
}
Operations2 = [
  UnaryOp(Variables2['i'],  Variables2['i1'], 1, 0, SymbolicInterval(Variables2['j'], '<')),
  UnaryOp(Variables2['i1'], Variables2['i2'], b=1),
  UnaryOp(Variables2['i2'], Variables2['i']),
  UnaryOp(Variables2['j'],  Variables2['j1'], 1, 0, SymbolicInterval(Variables2['i'], '>')),
  UnaryOp(Variables2['j1'], Variables2['j2'], b = -1),
  UnaryOp(Variables2['j2'], Variables2['j'])
]
UseMap2 = {
  'i' : [Operations2[0]],
  'i1': [Operations2[1]],
  'i2': [Operations2[2]],
  'j' : [Operations2[3]],
  'j1': [Operations2[4]],
  'j2': [Operations2[5]]
}

# Graph 3:
Variables3 = {
  'a': VarNode('a', BottomInterval()),
  'b': VarNode('b', Interval(0, 0)),
  'c': VarNode('c', BottomInterval())
}
Operations3 = [
  UnaryOp(Variables3['a'],  Variables3['b'], 1, 0, Interval('-', 100)),
  UnaryOp(Variables3['b'],  Variables3['c'], 1, 0),
  UnaryOp(Variables3['c'],  Variables3['a'], 1, 1)
]
UseMap3 = {
  'a': [Operations3[0]],
  'b': [Operations3[1]],
  'c': [Operations3[2]]
}

def toDot(Title, Variables, Operations):
  """Print the edges in dot format."""
  print 'digraph "' + Title + '" {'
  for v, k in Variables.iteritems():
    print " ", v, "[label=\"", str(k), "\"]"
  for op in Operations:
    print op.toDotStr()
  print '}'

def growth_meet(op):
  """This is the meet operator of the growth analysis."""
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

def crop_meet(op):
  """This is the meet operator of the cropping analysis."""
  new_int = op.eval()
  has_changed = False
  if op.sink.state == '-' and gt(new_int.l, op.sink.interval.l):
    op.sink.interval.l = new_int.l
    has_changed = True
  if op.sink.state == '+' and lt(new_int.u, op.sink.interval.u):
    op.sink.interval.u = new_int.u
    has_changed = True
  return has_changed

def iterate_till_fix_point(use_map, active_vars, meet):
  """This method finds the abstract state of each variable."""
  if len(active_vars) > 0:
    next_variable = active_vars.pop()
    use_list = use_map[next_variable]
    for op in use_list:
      if meet(op):
        active_vars.add(op.sink.name)
    iterate_till_fix_point(use_map, active_vars, meet)

def store_abstract_states(variables):
  """This method stores the abstract state of each variable. The possible
     states are '0', '+', '-' and '?'."""
  for var in variables:
    if var.interval.l == '-' and var.interval.u == '+':
      var.state = '?'
    elif var.interval.l == '-':
      var.state = '-'
    elif var.interval.u == '+':
      var.state = '+'
    else:
      var.state = '0'

def int_op(Variables, UseMap):
  """This method finds which operations contain non-trivial intersection."""
  non_trivial_set = set()
  for var_name in Variables:
    for op in UseMap[var_name]:
      if isinstance(op, UnaryOp) and (op.i.u != '+' or op.i.l != '-'):
        non_trivial_set.add(var_name)
        break
  return non_trivial_set
    
def findIntervals(Variables, Operations, UseMap, entryPoints):
  """Finds the intervals of a SCC."""
  toDot("First graph", Variables, Operations)
  iterate_till_fix_point(UseMap, set(entryPoints), growth_meet)
  toDot("After Growth Analysis", Variables, Operations)
  for op in Operations:
    op.fixIntersects()
  toDot("After fixing the intersections", Variables, Operations)
  store_abstract_states(Variables.values())
  iterate_till_fix_point(UseMap, int_op(Variables, UseMap), crop_meet)
  toDot("After cropping analysis", Variables, Operations)

#findIntervals(Variables2, Operations2, UseMap2, ['i', 'j'])
#findIntervals(Variables0, Operations0, UseMap0, ['k', 'k0'])
findIntervals(Variables3, Operations3, UseMap3, ['b'])
