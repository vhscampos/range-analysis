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
  def intersect(self, i):
    if (gt(self.l, i.u) or lt(self.u, i.l)):
      return BottomInterval()
    else:
      return Interval(max(self.l, i.l), min(self.u, i.u))
  def __str__(self):
    return "[" + str(self.l) + ", " + str(self.u) + "]"
  def isBottom(self):
    return False

class BottomInterval(Interval):
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

  def getInterval(self, init):
    """Returns an interval that is the application of this constraint on the
       interval [x, y]"""
    if (init.isBottom()):
      return BottomInterval()
    l = add(mul(self.a, init.l), self.b)
    u = add(mul(self.a, init.u), self.b)
    i = Interval(l, u)
    return i.intersect(self.i)

  def compose(self, f):
    a = self.a * f.a
    b = f.a * self.b + f.b
    i = Interval(mul(f.a, self.i.l), mul(f.a, self.i.u)).intersect(f.i)
    return Function(a, b, i)

  def __str__(self):
    return "f(X) = " + str(self.a) + "*X + " + str(self.b) + " \int " + str(self.i)

def comp (l): return reduce(lambda f, g: f.compose(g), l)

def apply (l, init): return reduce(lambda i, f: f.getInterval(i), l, init)

xy = Function(1, 1)
yz = Function(1, 0, Interval(2, 9))
zw = Function(3, 0)

def testComposition():
  """Asks the user to create intervals, and passes it to the functions."""
  while True:
    more = raw_input("Would you like to continue? (y/n) ")
    if more in ('y', 'ye', 'yes'):
      l = int(raw_input("Lower limit: "))
      u = int(raw_input("Upper limit: "))
      try:
        i = Interval(l, u)
        print "Composition: ", comp([xy, yz, zw]).getInterval(i)
        print "Application: ", apply([xy, yz, zw], i)
      except InvalidIntervalException:
        print "Invalid interval: [", l, ", ", u, "]. Please, ry again\n"
    else:
      break

testComposition()
