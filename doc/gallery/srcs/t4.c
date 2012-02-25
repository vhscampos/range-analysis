#include <stdio.h>

/*
 * This test shows how we handle comparisons between different types. These
 * comparisons are implemented via truncation by the compiler. We can use the
 * results of the truncate instruction to improve the bounds of some variables.
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
