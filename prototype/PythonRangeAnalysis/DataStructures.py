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

class ControlDep:
  """These constraints are used to simulate control dependence edges when
     running Nuutila's algorithm."""
  def __init__(self, source, sink):
    self.source = source
    self.sink = sink
  def getUseList(self):
    """Returns the list containting the single variable used in this unary
       operation"""
    return [self.source]
  def toDotStr(self):
    lb = "  " + str(hash(self)) + " [shape=box,label =\"DEP\"]\n"
    lb += "  " + self.source.name + " -> " + str(hash(self)) + "\n"
    lb += "  " + str(hash(self)) + " -> " + self.sink.name + "\n"
    return lb

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

def buildSymbolicIntersectMap(Operations):
  """This method builds a map of variables to the lists of operations where
     these variables are used as futures. Its C++ type should be something like
     map<VarNode, List<Operation>>."""
  FutureMap = {}
  for op in Operations:
    if isinstance(op, UnaryOp) and isinstance(op.i, SymbolicInterval):
      if FutureMap.has_key(op.i.bound):
        FutureMap[op.i.bound].append(op)
      else:
        FutureMap[op.i.bound] = [op]
  return FutureMap

def buildDefMap(Operations):
  """This method builds a map that binds each variable to the operation in
     which this variable is defined. Its C++ type should be something like
     map<VarNode, Operation>."""
  DefMap = {}
  for op in Operations:
    DefMap[op.sink] = op
  return DefMap

def buildUseMap(VarList, Operations):
  """This method builds a map that binds each variable label to the operations
     where this variable is used. Its C++ type should be something like
     map<VarNode, List<Operation>>."""
  UseMap = {}
  for var in VarList:
    uses = []
    for op in Operations:
      if var in op.getUseList():
        uses.append(op)
        continue
    UseMap[var.name] = uses
  return UseMap

def processGraph(fileName, findIntervals):
  """This method finds the intervals of the graph in the input file. The
     parameter 'fileName' is the name of the file that contains the graph in
     python format. The function 'findIntervals' is passed by the range
     analysis module, and contains the algorithm to find the ranges."""
  try:
    f = open(fileName, 'r')
    strFile = f.read()
    # The file contains the data in python format, so I can execute it
    # directly. This execution changes the Variables and Operations variables
    # in the internal scope of this file.
    exec(strFile)
    findIntervals(Variables, Operations)
  except IOError:
    print fileName, " is not a valid file name."

def readGraph(findIntervals):
  """This is the main test driver. It teads a file and process it. I have
     separated the processGraph function from the readGraph function just
     because when debugging the algorithms it is faster to open a file by
     hardwiring the code in the source file, instead of having to type the name
     of the file."""
  fileName = raw_input("Please, enter a file name: ")
  content = ""
  while fileName != "":
    processGraph(fileName, findIntervals)
    fileName = raw_input("Please, enter a file name: ")
  else:
    print content

def addControlDependenceEdges(FutureMap, UseMap):
  """Adds the edges that ensure that we solve a future before fixing its
     interval. I have created a new class: ControlDep edges, to represent
     the control dependences. In this way, in order to delete these edges,
     one just need to go over the map of uses removing every instance of the
     ControlDep class."""
  for var, opList in FutureMap.iteritems():
    for op in opList:
      UseMap[op.i.bound.name].append(ControlDep(op.i.bound, op.sink))

def delControlDependenceEdges(UseMap):
  """Removes the control dependence edges from the constraint graph."""
  for var, ops in UseMap.iteritems():
    while len(UseMap[var]) > 0 and isinstance(UseMap[var][-1], ControlDep):
      UseMap[var].pop()

def printInstance(Variables, Operations, UseMap, EntryPoints):
  """This method is used for debugging purposes only. I usually call it for
     each SCC, right before invoking the solver on it."""
  print "= = = = = = = = =\nVariables:"
  for v in Variables:
    print "  ", v
  print "Operations:"
  for op in Operations:
    print "  ", op
  print "Use map:"
  for v, ops in UseMap.iteritems():
    print "  ", v, ops
  print "Entry points:"
  for v in EntryPoints:
    print "  ", v

class Nuutila:
  """This class implements Nuutila's algorithm to find strongly connected
     components. The implementation of the class provides an iterator that goes
     over the SCCs in topological order. The class adds control dependence
     edges into the graph to produce the correct topological ordering of the
     SCCs."""
  def __init__(self, Variables, UseMap, FutureMap):
    """Finds the strongly connected components in the constraint graph formed by
       Variables and UseMap. The class receives the map of futures to insert the
       control dependence edges in the contraint graph. These edges are removed
       after the class is done computing the SCCs."""
    self.variables = Variables
    self.index = 0
    self.dfs = {}
    self.root = {}
    self.inComponent = set()
    self.components = {}
    self.worklist = []
    for var in Variables.keys():
      self.dfs[var] = -1
    addControlDependenceEdges(FutureMap, UseMap)
    for var in Variables:
      if self.dfs[var] < 0:
        self.visit(var, [], UseMap)
    delControlDependenceEdges(UseMap)
    
  def visit(self, v, stack, UseMap):
    """Finds SCCs using Nuutila's algorithm. This algorithm is divided in
       two parts. The first calls the recursive visit procedure on every node
       in the constraint graph. The second phase revisits these nodes,
       grouping them in components."""
    self.dfs[v] = self.index
    self.index += 1
    self.root[v] = v
    # Visit every node defined in an instruction that uses v:
    for w in UseMap[v]:
      if self.dfs[w.sink.name] < 0:
        self.visit(w.sink.name, stack, UseMap)
      if not w.sink.name in self.inComponent and self.dfs[self.root[v]] >= self.dfs[self.root[w.sink.name]]:
        self.root[v] = self.root[w.sink.name]
    # The second phase of the algorithm assigns components to stacked nodes:
    if self.root[v] == v:
      # Neither the worklist nor the map of components is part of Nuutila's
      # original algorithm. We are using these data structures to get a
      # topological ordering of the SCCs without having to go over the
      # root list once more.
      self.worklist.append(v)
      SCC = [self.variables[v]]
      self.inComponent.add(v)
      while len(stack) > 0 and self.dfs[stack[-1]] > self.dfs[v]:
        node = stack[-1]
        stack.pop()
        self.inComponent.add(node)
        SCC.append(self.variables[node])
      self.components[v] = SCC
    else: 
      stack.append(v)

  def printSCCs(self):
    """Prints the result of Nuutila's algorithm. This method is used for
       debugging purposes only. If one needs to go over the list of SCCs,
       it is better to use the iterator via a 'for each' loop."""
    for i in range(1, len(self.worklist) + 1):
      v = self.worklist[-i]
      print "SCC of:", v
      SCC = self.components[v]
      for w in SCC:
        print "  ", w

  def __iter__(self):
    """This method returns an iterator that produces the SCCs in topological
       order. To visit each component, just do a 'for c in nut', where 'nut' is
       the initialized instance of this class."""
    self.iterIndex = len(self.worklist)
    return self

  def next(self):
    """We traverse the worklist backwards to go over the SCCs in topological
       ordering."""
    if self.iterIndex <= 0:
      raise StopIteration
    self.iterIndex -= 1
    return self.components[self.worklist[self.iterIndex]]
