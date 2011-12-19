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
  def isBottom(self):
    return False

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
  def __init__(self):
    self.l = '.'
    self.u = '.'
  def isBottom(self):
    return True

class VarNode:
  """A VarNode represents a program variable."""
  def __init__(self, name, interval=Interval(), state='U'):
    self.name = name
    self.interval = interval
    self.state = state
  def __str__(self):
    return self.name + " (" + self.state + ") " + str(self.interval)

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
    if self.i.__class__.__name__ == SymbolicInterval.__name__:
      l = self.i.bound.interval.l
      u = self.i.bound.interval.u
      state = self.i.bound.state
      if self.i.op == '==':
        self.i = Interval(l, u)
      elif self.i.op == '<=' and (state == '-' or state == '0'):
        self.i = Interval(self.i.l, u)
      elif self.i.op == '<' and (state == '-' or state == '0'):
        self.i = Interval(self.i.l, u - 1)
      elif self.i.op == '>=' and (state == '+' or state == '0'):
        self.i = Interval(l, self.i.u)
      elif self.i.op == '>' and (state == '+' or state == '0'):
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
  'k1': VarNode('k1'),
  'k2': VarNode('k2')
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
  'k2': VarNode('k2'),
  'k3': VarNode('k3', Interval(7, 7)),
  'k4': VarNode('k4')
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
  'i1': VarNode('i1'),
  'i2': VarNode('i2'),
  'j': VarNode('j', Interval(100, 100)),
  'j1': VarNode('j1'),
  'j2': VarNode('j2'),
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
  print "Growing: " + str(op)
  old_interval = op.sink.interval
  new_interval = op.eval()
  old_state = op.sink.state
  print " Reading " + str(op.sink) + ", updating to ", str(new_interval)
  if old_state == "U":
    op.sink.state = "0"
  elif old_state == "0":
    if new_interval.l < old_interval.l and new_interval.u > old_interval.u:
      op.sink.state = '?'
    elif new_interval.l < old_interval.l:
      op.sink.state = '-'
    elif new_interval.u > old_interval.u:
      op.sink.state = '+'
  elif old_state == "+" and new_interval.l < old_interval.l:
      op.sink.state = '?'
  elif old_state == "-" and new_interval.u > old_interval.u:
      op.sink.state = '?'
  op.sink.interval = new_interval
  print " Writing " + str(op.sink)
  return op.sink.state

def growth_analysis(use_map, active_vars, meet):
  """This method finds the abstract state of each variable."""
  if len(active_vars) > 0:
    next_variable = active_vars.pop()
    use_list = use_map[next_variable]
    for op in use_list:
      old_state = op.sink.state
      new_state = meet(op)
      if new_state != old_state:
        active_vars.add(op.sink.name)
    growth_analysis(use_map, active_vars, meet)
    
def findIntervals(Variables, Operations, UseMap, entryPoints):
  """Finds the intervals of a SCC."""
  toDot("First graph", Variables, Operations)
  growth_analysis(UseMap, set(entryPoints), growth_meet)
  toDot("After Growth Analysis", Variables, Operations)
  for op in Operations:
    op.fixIntersects()
  toDot("After fixing the intersections", Variables, Operations)

findIntervals(Variables2, Operations2, UseMap2, ['i', 'j'])
# findIntervals(Variables0, Operations0, UseMap0, ['k', 'k0'])
