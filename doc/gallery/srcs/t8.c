/*
 * This file illustrates the fact that mutually recursive functions might
 * create some very large SCC's in the interprocedural analysis. These functions
 * cannot be inlined, so, in the end, we must match formal and actual
 * parameters.
 */

#include <stdio.h>

int fun0(int a, int b) {
  if (a > b) {
    return fun1(a - 1, b + 1);
  } else {
    return b - a;
  }
}

int fun1(int c, int d) {
  if (c > d) {
    return fun2(c - 1, d + 1);
  } else {
    return d - c;
  }
}

int fun2(int e, int f) {
  if (e > f) {
    return fun3(e - 1, f + 1);
  } else {
    return f - e;
  }
}

int fun3(int g, int h) {
  if (g > h) {
    return fun4(g - 1, h + 1);
  } else {
    return h - g;
  }
}

int fun4(int i, int j) {
  if (i > j) {
    return fun0(i - 1, j + 1);
  } else {
    return j - i;
  }
}

int main(int argc, char** argv) {
  int i0 = argv[0][0];
  int i1 = argv[0][1];
  printf("%d\n", fun0(i0, i1));
}
