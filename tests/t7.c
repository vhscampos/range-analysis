#include <stdio.h>

/*
 * A test case to check we can handle a two-deep nested call.
 */
int bar(int i, int j) {
  while (i < j) {
    i = i + 1;
    j = j - 1;
  }
}

int foo(int k, int N) {
  while (k < N) {
    bar(0, k);
    k = k + 1;
  }
  return k;
}

int main(int argc, char** argv) {
  printf("%d\n", foo(0, 100));
}
