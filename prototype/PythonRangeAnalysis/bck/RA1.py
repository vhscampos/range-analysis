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
      return "[lb(" + self.bound + "), ub(" + self.bound + ")]"
    elif self.op == '<=':
      return "[-, ub(" + self.bound + ")]"
    elif self.op == '<':
      return "[-, ub(" + self.bound + ") - 1]"
    elif self.op == '>=':
      return "[lb(" + self.bound + "), +]"
    elif self.op == '>':
      return "[lb(" + self.bound + ") + 1, +]"
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

class Function:
  def __init__(self, a=1, b=0, interval=Interval('-', '+')):
    self.a = a
    self.b = b
    self.i = interval

  def eval(self, seed):
    """Returns an interval that is the application of this constraint on the
       interval [x, y]"""
    if (seed.isBottom()):
      return BottomInterval()
    l = add(mul(self.a, seed.l), self.b)
    u = add(mul(self.a, seed.u), self.b)
    i = Interval(l, u)
    return i.intersection(self.i)

  def __str__(self):
    return "f(X) = " + str(self.a) + "*X + " + str(self.b) + " \int " + str(self.i)

  def toLabel(self):
    lb = ""
    space = ""
    if self.a != 1:
      lb += "*(" + str(self.a) + ")"
      space = " "
    if self.b != 0:
      lb += space + "+(" + str(self.b) + ")"
      space = " "
    if self.i.__class__.__name__ == SymbolicInterval.__name__ or self.i.l != '-' or self.i.u != '+':
      lb += space + "INT" + str(self.i)
    return lb

# Graph 0:
Valuation0 = {
  'k' : Interval(0, 0),
  'k1': Interval(),
  'k2': Interval()
}
Status0 = {
  'k' : '0',
  'k1': '0',
  'k2': '0'
}
Constraint0 = {
  ('k', 'k1') : Function(1, 0, Interval('-', 100)),
  ('k1', 'k2'): Function(1, 1, Interval()),
  ('k2', 'k') : Function(1, 0, Interval())
}

# Graph 1:
Valuation1 = {
  'i0': Interval(4, 5),
  'i1': Interval(),
  'i2': Interval(),
  'i3': Interval()
}
Status1 = {
  'i0': '0',
  'i1': '0',
  'i2': '0',
  'i3': '0'
}
Constraint1 = {
  ('i0', 'i1'): Function(b = -3),
  ('i0', 'i2'): Function(b = 2),
  ('i0', 'i3'): Function(2, -1),
  ('i1', 'i2'): Function(b = 3),
  ('i3', 'i2'): Function(b = -3),
  ('i2', 'i0'): Function()
}

# Graph 2:
Valuation2 = {
  'i' : Interval(0, 0),
  'i1': Interval(),
  'i2': Interval(),
  'j' : Interval(100, 100),
  'j1': Interval(),
  'j2': Interval()
}
Status2 = {
  'i' : '0',
  'i1': '0',
  'i2': '0',
  'j' : '0',
  'j1': '0',
  'j2': '0'
}
Constraint2 = {
  ('i', 'i1') : Function(1, 0, SymbolicInterval('j', '<')),
  ('i1', 'i2'): Function(b = 1),
  ('i2', 'i') : Function(),
  ('j', 'j1') : Function(1, 0, SymbolicInterval('i', '>=')),
  ('j1', 'j2'): Function(b = -1),
  ('j2', 'j') : Function()
}

def copyDic(d):
  newDic = {}
  for k, v in d.iteritems():
    newDic[k] = v
  return newDic

def update((u, v), Valuation, Status, Constraint):
  new_interval = Valuation[v].union(Constraint[(u, v)].eval(Valuation[u]))
  Valuation[v] = new_interval

def propagate(Valuation, Constraint, s, Done):
  """Propagate a initial interval to every node in V, starting from s."""
  for (u, v) in Constraint.keys():
    if u == s and not Done.__contains__(v):
      Valuation[v] = Valuation[u].union(Constraint[(u, v)].eval(Valuation[u]))
      Done.add(v)
      propagate(Valuation, Constraint, v, Done)

def pump(Valuation, Status, Constraint):
  """Performs Bellman-Ford relaxation on the edges of this constraint graph."""
  oldRanges = copyDic(Valuation)
  for i in range(len(Valuation)):
    for e in Constraint.keys():
      update(e, Valuation, Status, Constraint)
  for vertice, interval in Valuation.iteritems():
    oldInterval = oldRanges[vertice]
    if interval.l < oldInterval.l and interval.u > oldInterval.u:
      Status[vertice] = '?'
    elif interval.l < oldInterval.l:
      Status[vertice] = '-'
    elif interval.u > oldInterval.u:
      Status[vertice] = '+'

def fixIntersects(Valuation, Status, Constraints):
  """Replaces symbolic bounds with actual bounds."""
  for edge, func in Constraints.iteritems():
    interv = func.i
    if interv.__class__.__name__ == SymbolicInterval.__name__:
      status = Status[interv.bound]
      if interv.op == '==':
        func.i = Interval(Valuation[interv.bound].l,Valuation[interv.bound].u)
      elif interv.op == '<=' and (status == '-' or status == '0'):
        func.i = Interval(interv.l, Valuation[interv.bound].u)
      elif interv.op == '<' and (status == '-' or status == '0'):
        func.i = Interval(interv.l, Valuation[interv.bound].u - 1)
      elif interv.op == '>=' and (status == '+' or status == '0'):
        func.i = Interval(Valuation[interv.bound].l, interv.u)
      elif interv.op == '>' and (status == '+' or status == '0'):
        func.i = Interval(Valuation[interv.bound].l + 1, interv.u)
      else:
        func.i = Interval()

def toDot(Valuation, Status, Constraint):
  """Print the edges in dot format."""
  print 'digraph "Constraint Graph" {'
  for v in Valuation:
    print "  ", v, "[label=\"", v, Valuation[v], Status[v], "\"]"
  for (u, v) in Constraint.keys():
    print "  ", u, " -> ", v, " [label=\"", Constraint[(u, v)].toLabel(), "\"]"
  print '}'

Valuation = Valuation2
Status = Status2
Constraints = Constraint2

toDot(Valuation, Status, Constraints)

propagate(Valuation, Constraints, 'i', set(['i']))
propagate(Valuation, Constraints, 'j', set(['j']))

toDot(Valuation, Status, Constraints)

pump(Valuation, Status, Constraints)

toDot(Valuation, Status, Constraints)

fixIntersects(Valuation, Status, Constraints)

toDot(Valuation, Status, Constraints)
