#include <stdio.h>

/*
 * The basic case: the inter and intra-procedural analyses should return the
 * same results.
 */
int foo() {
  int k = 0;
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
  printf("%d\n", foo());
}
