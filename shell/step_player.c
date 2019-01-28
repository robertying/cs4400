#include <stdio.h>
#include "io.h"

int main(int argc, char **argv)
{
  int x, y, g;

  read_position(argv[0], &x, &y);

  /* toward bigger x */
  while (x < MAX_POSITION) {
    write_position(++x, y);
    g = read_guidance(argv[0]);
    if (g == WINNER) return 0; /* won! */
    if (g < STEADY) break;
  }

  /* toward smaller x */
  while (x > 0) {
    write_position(--x, y);
    g = read_guidance(argv[0]);
    if (g == WINNER) return 0; /* won! */
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
    if (g == WINNER) return 0; /* won! */
    if (g < STEADY) break;
  }

  /* toward smaller y */
  while (y > 0) {
    write_position(x, --y);
    if (read_guidance(argv[0]) == WINNER)
      return 0; /* won! */
  }

  return 1;
}
