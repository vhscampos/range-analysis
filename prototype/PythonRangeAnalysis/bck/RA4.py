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
  def __init__(self, name, interval=Interval(), status='0'):
    self.name = name
    self.interval = interval
    self.status = status
  def __str__(self):
    return self.name + " (" + self.status + ") " + str(self.interval)

class UnaryOp:
  """A constraint like sink = a * source + b \intersec [l, u]"""
  def __init__(self, source, sink, a=1, b=0, interval=Interval('-', '+')):
    self.source = source
    self.sink = sink
    self.a = a
    self.b = b
    self.i = interval
  def init(self):
    """Initializes the value in the sink from the value in the source."""
    l = add(mul(self.a, self.source.interval.l), self.b)
    u = add(mul(self.a, self.source.interval.u), self.b)
    self.sink.interval = Interval(l, u).intersection(self.i)
  def update(self):
    """Reads the interval in source, apply the operation on it, and propagates
       it to sink."""
    l = add(mul(self.a, self.source.interval.l), self.b)
    u = add(mul(self.a, self.source.interval.u), self.b)
    auxInterval = Interval(l, u)
    self.sink.interval = self.sink.interval.union(auxInterval.intersection(self.i))
  def saturate(self):
    """Sends the largest possible interval to the sink."""
    satInterval = Interval()
    if self.source.status == '0':
       satInterval = Interval(self.source.interval.l, self.source.interval.u)
    elif self.source.status == '+':
       satInterval = Interval(self.source.interval.l, '+')
    elif self.source.status == '-':
       satInterval = Interval('-', self.source.interval.u)
    self.sink.interval = self.sink.interval.union(satInterval.intersection(self.i))
  def __str__(self):
    return str(self.sink) + " = " + str(self.a) + "* " + str(self.source) + str(self.b) + " \int " + str(self.i)
  def fixIntersects(self):
    """Replace symbolic intervals with hard-wired constants."""
    if self.i.__class__.__name__ == SymbolicInterval.__name__:
      l = self.i.bound.interval.l
      u = self.i.bound.interval.u
      status = self.i.bound.status
      if self.i.op == '==':
        self.i = Interval(l, u)
      elif self.i.op == '<=' and (status == '-' or status == '0'):
        self.i = Interval(self.i.l, u)
      elif self.i.op == '<' and (status == '-' or status == '0'):
        self.i = Interval(self.i.l, u - 1)
      elif self.i.op == '>=' and (status == '+' or status == '0'):
        self.i = Interval(l, self.i.u)
      elif self.i.op == '>' and (status == '+' or status == '0'):
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
    if self.i.__class__.__name__ == SymbolicInterval.__name__ or self.i.l != '-' or self.i.u != '+':
      lb += space + "INT" + str(self.i)
    lb += "\"]\n"
    lb += "  " + self.source.name + " -> " + str(hash(self)) + "\n"
    lb += "  " + str(hash(self)) + " -> " + self.sink.name + "\n"
    return lb

class PlusOp:
  """A constraint like sink = a * source + b \intersec [l, u]"""
  def __init__(self, source1, source2, sink):
    self.source1 = source1
    self.source2 = source2
    self.sink = sink
  def init(self):
    """Initializes the value in the sink from the values in the sources."""
    int1 = self.source1.interval
    int2 = self.source2.interval
    self.sink.interval = addItv(int1, int2)
  def update(self):
    """Reads the interval in source, apply the operation on it, and propagates
       it to sink."""
    int1 = self.source1.interval
    int2 = self.source2.interval
    intSk = addItv(int1, int2)
    self.sink.interval = self.sink.interval.union(intSk)
  def saturate(self):
    """Sends the largest possible interval to the sink."""
    satInterval = Interval()
    l1 = self.source1.interval.l
    l2 = self.source2.interval.l
    u1 = self.source1.interval.u
    u2 = self.source2.interval.u
    if self.source1.status == '0' and self.source2.status == '0':
      satInterval = addItv(self.source1.interval, self.source2.interval)
    elif self.source1.status == '+' and self.source2.status == '0':
      satInterval = Interval(add(l1, l2), '+')
    elif self.source1.status == '-' and self.source2.status == '0':
      satInterval = Interval('-', add(u1, u2))
    elif self.source1.status == '0' and self.source2.status == '+':
      satInterval = Interval(add(l1, l2), '+')
    elif self.source1.status == '0' and self.source2.status == '-':
      satInterval = Interval('-', add(u1, u2))
    elif self.source1.status == '+' and self.source2.status == '+':
      satInterval = Interval(add(l1, l2), '+')
    elif self.source1.status == '-' and self.source2.status == '-':
      satInterval = Interval('-', add(u1, u2))
    self.sink.interval = self.sink.interval.union(satInterval)
  def __str__(self):
    return self.sink.name + " = " + self.source1.name + " + " + self.source2.name
  def fixIntersects(self):
    """Replace symbolic intervals with hard-wired constants. Normally this
       kind of operations have no intersect to fix, but the method is here so
       that we can invoke it on any kind of operation."""
  def toDotStr(self):
    lb = "  " + str(hash(self)) + " [label =\" + \"]\n"
    lb += "  " + self.source1.name + " -> " + str(hash(self)) + "\n"
    lb += "  " + self.source2.name + " -> " + str(hash(self)) + "\n"
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

def init(UseMap, ActiveVars, Done):
  """Finds an initial interval to every variable in the SCC."""
  for variableName in ActiveVars:
     opList = UseMap[variableName]
     newActiveVars = []
     for op in opList:
       if not op.sink.name in Done:
         op.init()
         Done.add(op.sink.name)
         newActiveVars.append(op.sink.name)
     init(UseMap, newActiveVars, Done)

def update(UseMap, ActiveVars, Done):
  """Saturates the intervals of every vertex."""
  for variableName in ActiveVars:
     opList = UseMap[variableName]
     newActiveVars = []
     for op in opList:
       if not op.sink.name in Done:
         op.update()
         Done.add(op.sink.name)
         newActiveVars.append(op.sink.name)
     update(UseMap, newActiveVars, Done)

def pump(Operations):
  """Applies the Bellman-Ford relaxations on the constraint set"""
  for i in range(len(Variables1)):
    for op in Operations:
      oldInterval = op.sink.interval
      op.update()
      newInterval = op.sink.interval
      if newInterval.l < oldInterval.l and newInterval.u > oldInterval.u:
        op.sink.status = '?'
      elif newInterval.l < oldInterval.l:
        op.sink.status = '-'
      elif newInterval.u > oldInterval.u:
        op.sink.status = '+'

def findIntervals(Variables, Operations, UseMap, entryPoints):
  """Finds the intervals of a SCC."""
  toDot("First graph", Variables, Operations)
  init(UseMap, entryPoints, set(entryPoints))
  toDot("After init", Variables, Operations)
  pump(Operations)
  toDot("After Bellman-Ford", Variables, Operations)
  for op in Operations:
    op.fixIntersects()
  toDot("After fixing the intersections", Variables, Operations)
  startingConstraints = set()
  for varName in entryPoints:
    startingConstraints.update(UseMap[varName])
  for op in startingConstraints:
    op.saturate()
  update(UseMap, entryPoints, set())
  toDot("After saturation", Variables, Operations)

findIntervals(Variables2, Operations2, UseMap2, ['i', 'j'])
# findIntervals(Variables0, Operations0, UseMap0, ['k', 'k0'])
