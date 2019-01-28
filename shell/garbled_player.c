#include <stdlib.h>
#include <stdio.h>
#include "io.h"

int main(int argc, char **argv)
{
  int x, y, i, n, g;

  read_position(argv[0], &x, &y);

  /* For some number of steps, hover at start position */
  n = x+y;
  for (i = 0; i < n; i++) {
    if (x < MAX_POSITION)
      write_position(++x, y);
    else
      write_position(--x, y);
    g = read_guidance(argv[0]);
    if (g == WINNER) return 1;
  }

  /* Print a bogus partial message: */
  printf("%d\n", x-y);

  return 0;
}
