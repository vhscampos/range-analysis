/*
 * This program illustrates a situation in which an one-iteration widening is
 * imprecise. The benchmark has a SCC with two entry-points that are phis, and
 * single iteration followed by widening leads to very conservative bounds.
 */
int main(int argc, char** argv) {
  int tooLong = 0;
  while (argc) {
    if (argc) {
      tooLong = 1;
    }
  }
  return tooLong;
}

