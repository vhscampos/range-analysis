#include <stdio.h>

/*
 * This test is just to check if we are handling comparisons between different
 * types correctly.
 */
int foo(char k) {
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
