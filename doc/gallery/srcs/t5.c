#include <stdio.h>

/*
 * A second case where the inter-procedural analysis should do better than the
 * intra-procedural. In this case, we have two arguments.
 */
int foo(char k, int N) {
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
  printf("%d\n", foo(0, 100));
}
