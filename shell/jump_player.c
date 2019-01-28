#include <stdio.h>
#include <stdlib.h>
#include "io.h"

static int jump(const char *who, int a, int b, int a_is_x)
{
  int lo, hi, amt, g;

  /* invariant: a is between lo and hi */
  lo = 0;
  hi = MAX_POSITION;

  while (lo < hi) {
    /* toward bigger a */
    amt = 1;
    while (a < hi) {
      if (a + amt > hi)
        amt = hi - a;
      a += amt;
      if (a_is_x)
        write_position(a, b);
      else
        write_position(b, a);
      g = read_guidance(who);
      if (g == WINNER) exit(0); /* won! */
      if (g <= STEADY) {
        /* colder or steady, so we know not to go past a anymore;
           switch to moving toward smaller a */
        hi = a;
        break;
      } else {
        /* halfway from old position is a lower bound */
        if (amt == 1)
          lo = a;
        else
          lo = a - amt/2;
        /* try a bigger jump */
        amt *= 2;
      }
    }

    /* toward smaller a */
    amt = 1;
    while (a > lo) {
      if (a - amt < 0)
        amt = a;
      a -= amt;
      if (a_is_x)
        write_position(a, b);
      else
        write_position(b, a);
      g = read_guidance(who);
      if (g == WINNER) exit(0); /* won! */
      if (g <= STEADY) {
        /* steady or colder, so don't go beyond a anymore */
        lo = a;
        break;
      } else {
        /* halfway from old position is an upper bound */
        if (amt == 1)
          hi = a;
        else
          hi = a + amt/2;
        /* try a bigger jump */
        amt *= 2;
      }
    }
  }

  return a;
}

int main(int argc, char **argv)
{
  int x, y;

  read_position(argv[0], &x, &y);

  x = jump(argv[0], x, y, 1);
  y = jump(argv[0], y, x, 0);

  /* should win before getting here */
  fprintf(stderr, "inconsistent guidance\n");
  return 1;
}
