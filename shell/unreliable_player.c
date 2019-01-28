#include <stdlib.h>
#include <stdio.h>
#include "io.h"

/* After this many steps, go wrong */
#define JUMP_TIME 45

int change(int x)
{
  if (x > 0)
    return x - 1;
  else
    return x + 1;
}

static int won(int init_x, int init_y)
{
  /* If this player started near the diagonal, fumble
     the loss by returning a non-success result. */
  if (abs(init_x - init_y) < 5)
    return 1;

  /* If this player started with a low x, fumble
     the loss by aborting the process. */
  if (init_x < 10)
    abort();

  return 0;
}

int main(int argc, char **argv)
{
  int init_x, init_y, x, y, g;
  int count = 0;

  read_position(argv[0], &x, &y);
  init_x = x;
  init_y = y;

  /* toward bigger x */
  while (x < MAX_POSITION) {
    if (++count == JUMP_TIME) y = change(y);
    write_position(++x, y);
    g = read_guidance(argv[0]);
    if (g == WINNER) return won(init_x, init_y);
    if (g < STEADY) break;
  }

  /* toward smaller x */
  while (x > 0) {
    if (++count == JUMP_TIME) y = change(y);
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
    if (++count == JUMP_TIME) x = change(x);
    write_position(x, ++y);
    g = read_guidance(argv[0]);
    if (g == WINNER) return won(init_x, init_y);
    if (g < STEADY) break;
  }

  /* toward smaller y */
  while (y > 0) {
    if (++count == JUMP_TIME) x = change(x);
    write_position(x, --y);
    if (read_guidance(argv[0]) == WINNER)
      return won(init_x, init_y);
  }

  return 1;
}
