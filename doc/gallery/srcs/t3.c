#include <stdio.h>

/*
 * In this case, we should be able to restrict k inside the loop, although the
 * function must be initialized with [-inf, +inf].
 */
int foo(int k) {
  while (k < 100) {
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
  printf("%d\n", foo(argc));
}
