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
  def getUseList(self):
    """Returns the list containting the single variable used in this unary
       operation"""
    return [self.source]
  def __str__(self):
    self_str = str(self.sink) + " = " + str(self.a) + "* " + str(self.source)
    self_str += str(self.b) + " \int " + str(self.i)
    return self_str
  def eval(self):
    """Read the interval in source, apply the operation on it, and return it."""
    l = add(mul(self.a, self.source.interval.l), self.b)
    u = add(mul(self.a, self.source.interval.u), self.b)
    if gt(l, u):
      auxInterval = Interval(u, l)
    else:
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
        self.i = Interval(self.i.l, add(u, -1))
      elif self.i.op == '>=':
        self.i = Interval(l, self.i.u)
      elif self.i.op == '>':
        self.i = Interval(add(l, 1), self.i.u)
      else:
        self.i = Interval()
  def toDotStr(self):
    lb = "  " + str(hash(self)) + " [shape=box,label =\""
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
  """A constraint like sink = src1 + src2"""
  def __init__(self, src1, src2, sink):
    self.src1 = src1
    self.src2 = src2
    self.sink = sink
  def getUseList(self):
    """Returns the list of variables used in this binary operation."""
    return [self.src1, self.src2]
  def __str__(self):
    return self.sink.name + " = " + self.src1.name + " + " + self.src2.name
  def eval(self):
    """Read the interval in source, apply the operation on it, and return it."""
    int1 = self.src1.interval
    int2 = self.src2.interval
    return Interval(add(int1.l, int2.l), add(int1.u, int2.u))
  def fixIntersects(self):
    """Replace symbolic intervals with hard-wired constants. Normally this
       kind of operations have no intersect to fix, but the method is here so
       that we can invoke it on any kind of operation."""
  def toDotStr(self):
    lb = "  " + str(hash(self)) + " [shape=box,label =\" + \"]\n"
    lb += "  " + self.src1.name + " -> " + str(hash(self)) + "\n"
    lb += "  " + self.src2.name + " -> " + str(hash(self)) + "\n"
    lb += "  " + str(hash(self)) + " -> " + self.sink.name + "\n"
    return lb

class PhiOp:
  """A constraint like sink = phi(src1, src2)"""
  def __init__(self, src1, src2, sink):
    self.src1 = src1
    self.src2 = src2
    self.sink = sink
  def getUseList(self):
    """Return the variables used in this phi node."""
    return [self.src1, self.src2]
  def __str__(self):
    return str(self.sink) + " =phi (" + str(self.src1) + ", " + str(self.src2) + ")"
  def eval(self):
    """The result of evaluating the phi-function is the union of the ranges of
       every variable used in the phi."""
    int1 = self.src1.interval
    int2 = self.src2.interval
    # Remember, the union of bottom and anythin is anything:
    if isinstance(int1, BottomInterval):
       return Interval(int2.l, int2.u)
    elif isinstance(int2, BottomInterval):
       return Interval(int1.l, int1.u)
    return Interval(min(int1.l, int2.l), max(int1.u, int2.u))
  def fixIntersects(self):
    """Replace symbolic intervals with hard-wired constants. Normally this
       kind of operations have no intersect to fix, but the method is here so
       that we can invoke it on any kind of operation."""
  def toDotStr(self):
    lb = "  " + str(hash(self)) + " [shape=box,label =\" phi \"]\n"
    lb += "  " + self.src1.name + " -> " + str(hash(self)) + "\n"
    lb += "  " + self.src2.name + " -> " + str(hash(self)) + "\n"
    lb += "  " + str(hash(self)) + " -> " + self.sink.name + "\n"
    return lb

def toDot(Title, Variables, Operations):
  """Print the edges in dot format."""
  print 'digraph "' + Title + '" {'
  for v, k in Variables.iteritems():
    print " ", v, "[label=\"", str(k), "\"]"
  for op in Operations:
    print op.toDotStr()
  print '}'

def buildUseMap(Variables, Operations):
  """This method builds a map that binds each variable label to the operations
     where this variable is used."""
  map = {}
  for var in Variables.values():
    uses = []
    for op in Operations:
      if var in op.getUseList():
        uses.append(op)
        continue
    map[var.name] = uses
  return map

def processGraph(fileName, findIntervals):
  """This method finds the intervals of the graph in the input file."""
  try:
    f = open(fileName, 'r')
    strFile = f.read()
    exec(strFile)
    UseMap = buildUseMap(Variables, Operations)
    EntryPoints = [var.name for var in Variables.values() if not isinstance(var.interval, BottomInterval)]
    findIntervals(Variables, Operations, UseMap, EntryPoints)
  except IOError:
    print fileName, " is not a valid file name."

def readGraph(findIntervals):
  """Reads a file and converts it into data structures."""
  fileName = raw_input("Please, enter a file name: ")
  content = ""
  while fileName != "":
    processGraph(fileName, findIntervals)
    fileName = raw_input("Please, enter a file name: ")
  else:
    print content

# The global index used to find the SCCs via Nuutila's algorithm.
class Nuutila:
  def __init__(self, Variables, UseMap, EntryPoints):
    """Finds the strongly connected components in the constraint graph formed by
       Variables and UseMap."""
    self.index = 0
    self.dfs = {}
    self.root = {}
    self.inComponent = set()
    for var in Variables.keys():
      self.dfs[var] = -1
    for var in EntryPoints:
      self.visit(var, [], UseMap)
    
  def visit(self, v, stack, UseMap):
    """Finds SCCs using Nuutila's algorithm."""
    self.dfs[v] = self.index
    print "DFS[", v, "] = ", self.index
    self.index += 1
    self.root[v] = v
    # Visit every node defined in an instruction that uses v:
    for w in UseMap[v]:
      if self.dfs[w.sink.name] < 0:
        self.visit(w.sink.name, stack, UseMap)
      if not w.sink.name in self.inComponent and self.dfs[self.root[v]] >= self.dfs[self.root[w.sink.name]]:
        self.root[v] = self.root[w.sink.name]
        print "1) Setting root[", v, "] = ", self.root[w.sink.name]
    # The second phase of the algorithm assigns components to stacked nodes:
    if self.root[v] == v:
      self.inComponent.add(v)
      print "Adding to component ", v
      while len(stack) > 0:
        node = stack[-1]
        if self.dfs[node] > self.dfs[v]:
          break
        else:
          stack.pop()
          self.inComponent.add(node)
          print "Adding to component ", node
          self.root[node] = v
          print "2) Setting root[", node, "] = ", v
    stack.append(v)
    print "Stacking ", v

  def printSCCs(self):
    """Prints the result of Nuutila's algorithm."""
    for k, v in self.root.iteritems():
      print k, v
