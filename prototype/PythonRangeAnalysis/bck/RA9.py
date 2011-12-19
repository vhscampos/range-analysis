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
  def __str__(self):
    self_str = str(self.sink) + " = " + str(self.a) + "* " + str(self.source)
    self_str += str(self.b) + " \int " + str(self.i)
    return self_str
  def eval(self, cut=True):
    """Read the interval in source, apply the operation on it, and return it."""
    l = add(mul(self.a, self.source.interval.l), self.b)
    u = add(mul(self.a, self.source.interval.u), self.b)
    auxInterval = Interval(l, u)
    if gt(l, u):
      auxInterval = Interval(u, l)
    else:
      auxInterval = Interval(l, u)
    if cut:
      return auxInterval.intersection(self.i)
    else:
      return auxInterval
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
        self.i = Interval(dd(l, +1), self.i.u)
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
  def __str__(self):
    return self.sink.name + " = " + self.src1.name + " + " + self.src2.name
  def eval(self, cut=True):
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
  def __str__(self):
    return self.sink.name + " =phi (" + str(self.src1) + ", " + str(self.src2) + ")"
  def eval(self, cut=True):
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

# Graph 4:
Variables4 = {
  'a': VarNode('a', Interval(1, 1)),
  'b': VarNode('b', Interval(0, 0)),
  'c': VarNode('c', BottomInterval()),
  'd': VarNode('d', BottomInterval()),
  'e': VarNode('e', BottomInterval())
}
Operations4 = [
  UnaryOp(Variables4['c'],  Variables4['a'], 1, -1),
  PlusOp(Variables4['a'], Variables4['b'], Variables4['c']),
  UnaryOp(Variables4['c'],  Variables4['d'], 1, 0, Interval(3, 7)),
  UnaryOp(Variables4['c'],  Variables4['e'], 1, 0, Interval('-', 10)),
  PhiOp(Variables4['d'], Variables4['e'], Variables4['b'])
]
UseMap4 = {
  'a': [Operations4[1]],
  'b': [Operations4[1]],
  'c': [Operations4[0], Operations4[2], Operations4[3]],
  'd': [Operations4[4]],
  'e': [Operations4[4]]
}

# Graph 5:
Variables5 = {
  'i0': VarNode('i0', Interval(0, 0)),
  'i1': VarNode('i1', BottomInterval()),
  'i2': VarNode('i2', BottomInterval()),
  'i3': VarNode('i3', BottomInterval()),
  'i4': VarNode('i4', BottomInterval()),
  'i5': VarNode('i5', BottomInterval()),
  'i6': VarNode('i6', BottomInterval()),
  'i7': VarNode('i7', BottomInterval())
}
Operations5 = [
  PhiOp(Variables5['i0'], Variables5['i6'], Variables5['i1']),
  UnaryOp(Variables5['i1'], Variables5['i2'], 1, 0, Interval('-', 41)),
  UnaryOp(Variables5['i1'], Variables5['i4'], 1, 0, Interval(42, '+')),
  UnaryOp(Variables5['i2'], Variables5['i3'], 1, 1),
  UnaryOp(Variables5['i4'], Variables5['i7'], 1, 0, Interval(43, '+')),
  UnaryOp(Variables5['i7'], Variables5['i5'], 1, 1),
  PhiOp(Variables5['i3'], Variables5['i5'], Variables5['i6'])
]
UseMap5 = {
  'i0': [Operations5[0]],
  'i1': [Operations5[1], Operations5[2]],
  'i2': [Operations5[3]],
  'i3': [Operations5[6]],
  'i4': [Operations5[4]],
  'i5': [Operations5[6]],
  'i6': [Operations5[0]],
  'i7': [Operations5[5]]
}

# Graph 6:
Variables6 = {
  'b0': VarNode('b0', BottomInterval()),
  'b1': VarNode('b1', BottomInterval()),
  'b2': VarNode('b2', BottomInterval()),
  'b3': VarNode('b3', BottomInterval()),
  'b4': VarNode('b4', BottomInterval()),
  'b5': VarNode('b5', Interval(1, 1)),
  'b6': VarNode('b6', BottomInterval())
}
Operations6 = [
  PhiOp(Variables6['b5'], Variables6['b4'], Variables6['b2']),
  PhiOp(Variables6['b3'], Variables6['b2'], Variables6['b6']),
  UnaryOp(Variables6['b6'], Variables6['b1'], 1, 2, Interval()),
  UnaryOp(Variables6['b6'], Variables6['b0'], 1, -1, Interval()),
  UnaryOp(Variables6['b0'], Variables6['b4'], 1, 0, Interval(-300, 800)),
  UnaryOp(Variables6['b1'], Variables6['b3'], 1, 0, Interval(-100, 500))
]
UseMap6 = {
  'b0': [Operations6[4]],
  'b1': [Operations6[5]],
  'b2': [Operations6[1]],
  'b3': [Operations6[1]],
  'b4': [Operations6[0]],
  'b5': [Operations6[0]],
  'b6': [Operations6[2], Operations6[3]]
}

# Graph 7:
Variables7 = {
  'a': VarNode('a', BottomInterval()),
  'b': VarNode('b', BottomInterval()),
  'c': VarNode('c', BottomInterval()),
  'd': VarNode('d', Interval(1, 1)),
  'e': VarNode('e', BottomInterval())
}
Operations7 = [
  PhiOp(Variables7['d'], Variables7['c'], Variables7['a']),
  PhiOp(Variables7['b'], Variables7['a'], Variables7['e']),
  UnaryOp(Variables7['e'], Variables7['b'], 1, 2, Interval(0, 500)),
  UnaryOp(Variables7['e'], Variables7['c'], 1, -3, Interval(3, 800)),
]
UseMap7 = {
  'a': [Operations7[1]],
  'b': [Operations7[1]],
  'c': [Operations7[0]],
  'd': [Operations7[0]],
  'e': [Operations7[2], Operations7[3]]
}

# Graph 8:
Variables8 = {
  'k0': VarNode('k0', Interval(0, 0)),
  'k1': VarNode('k1', BottomInterval()),
  'i0': VarNode('i0', Interval(0, 0)),
  'j0': VarNode('j0', BottomInterval()),
  'i1': VarNode('i1', BottomInterval()),
  'j1': VarNode('j1', BottomInterval()),
  'i2': VarNode('i2', BottomInterval()),
  'j2': VarNode('j2', BottomInterval()),
  'k2': VarNode('k2', BottomInterval())
}
Operations8 = [
  PhiOp(Variables8['k0'], Variables8['k2'], Variables8['k1']),
  PhiOp(Variables8['i0'], Variables8['i2'], Variables8['i1']),
  PhiOp(Variables8['j0'], Variables8['j2'], Variables8['j1']),
  UnaryOp(Variables8['k1'], Variables8['j0']),
  UnaryOp(Variables8['i1'], Variables8['i2'], 1, 1),
  UnaryOp(Variables8['j1'], Variables8['j2'], 1, -1),
  UnaryOp(Variables8['k1'], Variables8['k2'], 1, 1)
]
UseMap8 = {
  'i0': [Operations8[1]],
  'i1': [Operations8[4]],
  'i2': [Operations8[1]],
  'j0': [Operations8[2]],
  'j1': [Operations8[5]],
  'j2': [Operations8[2]],
  'k0': [Operations8[0]],
  'k1': [Operations8[3], Operations8[6]],
  'k2': [Operations8[0]]
}

# Graph 9:
Variables9 = {
  'k0': VarNode('k0', Interval(0, 0)),
  'k1': VarNode('k1', BottomInterval()),
  'i0': VarNode('i0', Interval(0, 0)),
  'j0': VarNode('j0', BottomInterval()),
  'i1': VarNode('i1', BottomInterval()),
  'j1': VarNode('j1', BottomInterval()),
  'i2': VarNode('i2', BottomInterval()),
  'j2': VarNode('j2', BottomInterval()),
  'k2': VarNode('k2', BottomInterval()),
  'kt': VarNode('kt', BottomInterval()),
  'it': VarNode('it', BottomInterval()),
  'jt': VarNode('jt', BottomInterval())
}
Operations9 = [
  PhiOp(Variables9['k0'], Variables9['k2'], Variables9['k1']),
  PhiOp(Variables9['i0'], Variables9['i2'], Variables9['i1']),
  PhiOp(Variables9['j0'], Variables9['j2'], Variables9['j1']),
  UnaryOp(Variables9['kt'], Variables9['j0']),
  UnaryOp(Variables9['it'], Variables9['i2'], 1, 1),
  UnaryOp(Variables9['jt'], Variables9['j2'], 1, -1),
  UnaryOp(Variables9['kt'], Variables9['k2'], 1, 1),
  UnaryOp(Variables9['k1'],  Variables9['kt'], 1, 0, Interval('-', 99)),
  UnaryOp(Variables9['i1'],  Variables9['it'], 1, 0, SymbolicInterval(Variables9
['j1'], '<')),
  UnaryOp(Variables9['j1'],  Variables9['jt'], 1, 0, SymbolicInterval(Variables9
['i1'], '>='))
]
UseMap9 = {
  'i0': [Operations9[1]],
  'i1': [Operations9[8]],
  'i2': [Operations9[1]],
  'it': [Operations9[4]],
  'j0': [Operations9[2]],
  'j1': [Operations9[9]],
  'j2': [Operations9[2]],
  'jt': [Operations9[5]],
  'k0': [Operations9[0]],
  'k1': [Operations9[7]],
  'k2': [Operations9[0]],
  'kt': [Operations9[3], Operations9[6]]
}

# Graph 10:
Variables10 = {
  'i0': VarNode('i0', Interval(0, 0)),
  'j0': VarNode('j0', BottomInterval()),
  'i1': VarNode('i1', BottomInterval()),
  'j1': VarNode('j1', BottomInterval()),
  'i2': VarNode('i2', BottomInterval()),
  'j2': VarNode('j2', BottomInterval()),
  'it': VarNode('it', BottomInterval()),
  'jt': VarNode('jt', BottomInterval()),
  'kt': VarNode('kt', Interval(0, 99))
}
Operations10 = [
  PhiOp(Variables10['i0'], Variables10['i2'], Variables10['i1']),
  PhiOp(Variables10['j0'], Variables10['j2'], Variables10['j1']),
  UnaryOp(Variables10['kt'], Variables10['j0']),
  UnaryOp(Variables10['it'], Variables10['i2'], 1, 1),
  UnaryOp(Variables10['jt'], Variables10['j2'], 1, -1),  UnaryOp(Variables10['i1'],  Variables10['it'], 1, 0, SymbolicInterval(Variables10['j1'], '<')),
  UnaryOp(Variables10['j1'],  Variables10['jt'], 1, 0, SymbolicInterval(Variables10['i1'], '>='))
]
UseMap10 = {
  'i0': [Operations10[0]],
  'i1': [Operations10[5]],
  'i2': [Operations10[1]],
  'it': [Operations10[3]],
  'j0': [Operations10[1]],
  'j1': [Operations10[6]],
  'j2': [Operations10[1]],
  'jt': [Operations10[4]],
  'kt': [Operations10[2]]
}

# Graph 11:
Variables11 = {
  'i0': VarNode('i0', Interval(0, 0)),
  'i1': VarNode('i1', BottomInterval()),
  'i2': VarNode('i2', BottomInterval()),
  'i3': VarNode('i3', BottomInterval()),
  'i4': VarNode('i4', BottomInterval())
}
Operations11 = [
  PhiOp(Variables11['i0'], Variables11['i3'], Variables11['i1']),
  UnaryOp(Variables11['i1'], Variables11['i2'], 1, 0, Interval('-', 99)),
  UnaryOp(Variables11['i2'], Variables11['i3'], 1, 1),
  UnaryOp(Variables11['i1'], Variables11['i4'], 1, 0, Interval(100, '+'))
]
UseMap11 = {
  'i0': [Operations11[0]],
  'i1': [Operations11[1], Operations11[3]],
  'i2': [Operations11[2]],
  'i3': [Operations11[0]],
  'i4': []
}

# Graph 12:
Variables12 = {
  'i0': VarNode('i0', Interval(0, 0)),
  'i1': VarNode('i1', BottomInterval()),
  'i2': VarNode('i2', BottomInterval())
}
Operations12 = [
  PhiOp(Variables12['i0'], Variables12['i2'], Variables12['i1']),
  UnaryOp(Variables12['i1'], Variables12['i2'], -1, 1)
]
UseMap12 = {
  'i0': [Operations12[0]],
  'i1': [Operations12[1]],
  'i2': [Operations12[0]]
}

# Graph 16:
Variables16 = {
  'i0': VarNode('i0', Interval(5, 5)),
  'i1': VarNode('i1', BottomInterval()),
  'i2': VarNode('i2', BottomInterval()),
  'i3': VarNode('i3', BottomInterval()),
  'i4': VarNode('i4', BottomInterval()),
  'i5': VarNode('i5', BottomInterval()),
  'i6': VarNode('i6', BottomInterval()),
  'i7': VarNode('i7', BottomInterval())
}
Operations16 = [
  PhiOp(Variables16['i0'], Variables16['i4'], Variables16['i5']),
  PhiOp(Variables16['i5'], Variables16['i6'], Variables16['i7']),
  UnaryOp(Variables16['i7'], Variables16['i1'], 1, 0, Interval(2, 11)),
  UnaryOp(Variables16['i1'], Variables16['i2'], 1, 0, Interval(1, 10)),
  UnaryOp(Variables16['i2'], Variables16['i3'], 1, 0, Interval(3, 12)),
  UnaryOp(Variables16['i3'], Variables16['i4'], 1, -1),
  UnaryOp(Variables16['i3'], Variables16['i6'], 1, 1)
]
UseMap16 = {
  'i0': [Operations16[0]],
  'i1': [Operations16[3]],
  'i2': [Operations16[4]],
  'i3': [Operations16[5], Operations16[6]],
  'i4': [Operations16[0]],
  'i5': [Operations16[1]],
  'i6': [Operations16[1]],
  'i7': [Operations16[2]]
}

# Graph 17:
Variables17 = {
  'i0': VarNode('i0', Interval(0, 0)),
  'i1': VarNode('i1', BottomInterval()),
  'i2': VarNode('i2', BottomInterval()),
  'i3': VarNode('i3', BottomInterval()),
  'i4': VarNode('i4', BottomInterval())
}
Operations17 = [
  PhiOp(Variables17['i0'], Variables17['i4'], Variables17['i1']),
  UnaryOp(Variables17['i1'], Variables17['i2'], 1, 0, Interval('-', 1000)),
  UnaryOp(Variables17['i2'], Variables17['i3'], 1, 0, Interval('-', 8)),
  UnaryOp(Variables17['i3'], Variables17['i4'], 1, 1)
]
UseMap17 = {
  'i0': [Operations17[0]],
  'i1': [Operations17[1]],
  'i2': [Operations17[2]],
  'i3': [Operations17[3],],
  'i4': [Operations17[0]]
}

def toDot(Title, Variables, Operations):
  """Print the edges in dot format."""
  print 'digraph "' + Title + '" {'
  for v, k in Variables.iteritems():
    print " ", v, "[label=\"", str(k), "\"]"
  for op in Operations:
    print op.toDotStr()
  print '}'

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
  new_int = op.eval(False)
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
      break
  return non_trivial_set
    
def findIntervals(Variables, Operations, UseMap, entryPoints):
  """Finds the intervals of a SCC."""
# toDot("First graph", Variables, Operations)
  iterate_till_fix_point(UseMap, set(entryPoints))
# toDot("After Growth Analysis", Variables, Operations)
  for op in Operations:
    op.fixIntersects()
# toDot("After fixing the intersections", Variables, Operations)
  store_abstract_states(Variables.values())
  for op in int_op(Operations):
    crop_dfs(UseMap, set([op]), set())
  toDot("After cropping analysis", Variables, Operations)

#findIntervals(Variables0, Operations0, UseMap0, ['k', 'k0'])
#findIntervals(Variables1, Operations1, UseMap1, ['k0', 'k1', 'k3'])
#findIntervals(Variables2, Operations2, UseMap2, ['i', 'j'])
#findIntervals(Variables3, Operations3, UseMap3, ['b'])
#findIntervals(Variables4, Operations4, UseMap4, ['a', 'b'])
#findIntervals(Variables5, Operations5, UseMap5, ['i0'])
#findIntervals(Variables6, Operations6, UseMap6, ['b5'])
#findIntervals(Variables7, Operations7, UseMap7, ['d'])
#findIntervals(Variables8, Operations8, UseMap8, ['k0', 'i0'])
#findIntervals(Variables9, Operations9, UseMap9, ['k0', 'i0'])
#findIntervals(Variables10, Operations10, UseMap10, ['kt', 'i0'])
#findIntervals(Variables11, Operations11, UseMap11, ['i0'])
#findIntervals(Variables12, Operations12, UseMap12, ['i0'])
findIntervals(Variables16, Operations16, UseMap16, ['i0'])
#findIntervals(Variables17, Operations17, UseMap17, ['i0'])
