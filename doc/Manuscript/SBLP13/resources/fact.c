#include <stdio.h>
#include <stdlib.h>

int fact(int n) {
  int r = 1;
  int i = 2;
  while (i <= n) {
    r *= i;
    i++;
  }
  return r;
}

int main(int argc, char** argv) {
  int n = atoi(argv[1]);
  printf("%d\n", n);
  printf("The factorial of %d is %d\n", n, fact(n));
}
