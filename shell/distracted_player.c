#include <stdlib.h>
#include <stdio.h>
#include "io.h"

int main(int argc, char **argv)
{
  int x, y, i, n, g;
  char buffer[16];

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

  if (x < MAX_POSITION)
    x++;
  else
    --x;

  /* Print a bogus partial message and close stdout: */
  sprintf(buffer, "%2.2d, %2.2d\n", x, y);
  fwrite(buffer, 1, (x+y) % 7, stdout);
  fclose(stdout);

  /* Loop forever after misbehaving: */
  while (1) { }
}
