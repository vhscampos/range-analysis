#include <stdio.h>

/*
 * This program shows that a context-insensitive analysis loses precision.
 * In this case, we can improve the results with inlining.
 */
int foo(int k, int N) {
  while (k < N) {
    int i = 0;
    int j = k;
    while (i < j) {
      i = i + 1;
      j = j - 1;
    }
    k = k + 1;
  }
  return k;
}

int main(int argc, char** argv) {
  printf("%d\n", foo(0, 10));
  printf("%d\n", foo(30, 100));
}
