#include <stdio.h>
#include "io.h"

int main(int argc, char **argv)
{
  int x, y;

  read_position(argv[0], &x, &y);

  /* Go to 0, 0 */
  
  if (x != 0) {
    write_position(0, y);
    if (read_guidance(argv[0]) == WINNER)
      return 0;
  }

  if (y != 0) {
    write_position(0, 0);
    if (read_guidance(argv[0]) == WINNER)
      return 0;
  }

  /* Walk across the board */

  y = 0;
  while (y <= MAX_POSITION) {
    int i;

    /* Jump to last x of this y, which is reachable
       from the previous position */
    write_position(MAX_POSITION, y);
    if (read_guidance(argv[0]) == WINNER)
      return 0;

    /* Walk across all x values for this y */
    for (i = 0; i <= MAX_POSITION; i++) {
      write_position(i, y);
      if (read_guidance(argv[0]) == WINNER)
        return 0;
    }

    /* We end at the last x for this y, which is adjacent
       to the last x of the next y */

    y++;
  }

  fprintf(stderr, "%s: tried the whole field\n", argv[0]);
  return 1;
}
