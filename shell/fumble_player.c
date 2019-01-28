#include <stdlib.h>
#include <stdio.h>
#include "io.h"

static int won(int init_x, int init_y)
{
  /* If this player started near the diagonal, fumble
     the loss by returning a non-success result. */
  if (abs(init_x - init_y) < 5)
    return 1;

  /* Otherwise, fumble the loss by aborting the process. */
  abort();

  return 0;
}

int main(int argc, char **argv)
{
  int init_x, init_y, x, y, g;

  read_position(argv[0], &x, &y);
  init_x = x;
  init_y = y;

  /* toward bigger x */
  while (x < MAX_POSITION) {
    write_position(++x, y);
    g = read_guidance(argv[0]);
    if (g == WINNER) return won(init_x, init_y);
    if (g < STEADY) break;
  }

  /* toward smaller x */
  while (x > 0) {
    write_position(--x, y);
    g = read_guidance(argv[0]);
    if (g == WINNER) return won(init_x, init_y);
    if (g < STEADY) {
      /* go back */
      write_position(++x, y);
      read_guidance(argv[0]);
      break;
    }
  }

  /* toward bigger y */
  while (y < MAX_POSITION) {
    write_position(x, ++y);
    g = read_guidance(argv[0]);
    if (g == WINNER) return won(init_x, init_y);
    if (g < STEADY) break;
  }

  /* toward smaller y */
  while (y > 0) {
    write_position(x, --y);
    if (read_guidance(argv[0]) == WINNER)
      return won(init_x, init_y);
  }

  return 1;
}
