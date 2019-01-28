#include <stdio.h>
#include <stdlib.h>
#include "io.h"
#include "random.h"

typedef struct player_t {
  int x, y, in_game;
} player_t;

static int random_position();
static int distance_to(player_t *player, int x, int y);

int main(int argc, char **argv)
{
  int n, seed, i, playing_n, min_init_distance, max_init_distance;
  int target_x, target_y;
  player_t *players;
  
  if (argc < 5) {
    fprintf(stderr, "%s: need <random-seed> <player-count> <min-distance> <max-distance> arguments\n", argv[0]);
    return 1;
  }

  seed = atoi(argv[1]);
  if (seed < 1) {
    fprintf(stderr, "%s: bad random seed, not a positive number: %s\n",
            argv[0], argv[1]);
    return 1;
  }

  n = atoi(argv[2]);
  if ((n < 1) || (n > 100)) {
    fprintf(stderr, "%s: bad player count, not a number between 1 and 100: %s\n",
            argv[0], argv[2]);
    return 1;
  }

  min_init_distance = atoi(argv[3]);
  if ((min_init_distance < 1) || (min_init_distance > (BOARD_SIZE-2))) {
    fprintf(stderr, "%s: bad minimum distance, not a number between 1 and %d: %s\n",
            argv[0], (BOARD_SIZE-2), argv[3]);
    return 1;
  }

  max_init_distance = atoi(argv[4]);
  if ((max_init_distance < 1) || (max_init_distance > (BOARD_SIZE-2))) {
    fprintf(stderr, "%s: bad maximum distance, not a number between 1 and %d: %s\n",
            argv[0], (BOARD_SIZE-2), argv[4]);
    return 1;
  }
  if (max_init_distance < min_init_distance) {
    fprintf(stderr, "%s: the maximum distance %d is smaller than the minimum distance %d\n",
            argv[0],
            max_init_distance,
            min_init_distance);
    return 1;
  }

  if (argc > 5)  {
    fprintf(stderr, "%s: too many command-line arguments; expected two arguments, but given %d\n",
            argv[0], argc - 1);
    return 1;
  }

  players = malloc(sizeof(player_t) * n);

  seed_random(seed);

  target_x = random_position();
  target_y = random_position();

  write_position(target_x, target_y);

  for (i = 0; i < n; i++) {
    int dist;
    do {
      int dx, dy;
      dist = min_init_distance;
      if (min_init_distance < max_init_distance)
        dist += next_random() % (max_init_distance - min_init_distance);
      
      dx = next_random() % dist;
      dy = dist - dx;
      if (next_random() & 0x1)
        dx = -dx;
      if (next_random() & 0x1)
        dy = -dy;
      players[i].x = target_x + dx;
      players[i].y = target_y + dy;
      if ((players[i].x < 0) || (players[i].x > MAX_POSITION)
          || (players[i].y < 0) || (players[i].y > MAX_POSITION)) {
        /* bad position; try again */
        dist = 0;
      } else
        dist = distance_to(&players[i], target_x, target_y);
    } while ((dist < min_init_distance) || (dist > max_init_distance));
    players[i].in_game = 1;
  }
  playing_n = n;

  /* report initial player locations */
  for (i = 0; i < n; i++) {
    write_position(players[i].x, players[i].y);
  }

  /* Loop until someone wins or all players go wrong */
  while (playing_n > 0) {
    for (i = 0; i < n; i++) {
      /* Only take moves for players that are still in the game: */
      if (players[i].in_game) {
        int new_x, new_y;
        char buffer[8];
        if ((fread(buffer, 1, 7, stdin) != 7)
            /* must be valid move syntax: */
            || !parse_position(buffer, &new_x, &new_y)
            /* must change x or y, but not both or neither: */
            || ((players[i].x != new_x) && (players[i].y != new_y))
            || ((players[i].x == new_x) && (players[i].y == new_y))) {
          /* bad move, so this player is out */
          printf("wrong!\n");
          fflush(stdout);
          players[i].in_game = 0;
          playing_n--;
        } else {
          /* move the player and report hotter/colder */
          int old_dist, new_dist;
          old_dist = distance_to(&players[i], target_x, target_y);
          players[i].x = new_x;
          players[i].y = new_y;
          new_dist = distance_to(&players[i], target_x, target_y);
          if (new_dist == 0) {
            printf("winner\n");
            /* player just won, so the game is over */
            return 0;
          } else if (new_dist < old_dist)
            printf("hotter\n");
          else if (new_dist > old_dist)
            printf("colder\n");
          else
            printf("steady\n");
          fflush(stdout);
        }
      }
    }
  }

  /* all players went wrong */
  return 1;
}

static int random_position()
{
  return next_random() % BOARD_SIZE;
}

static int distance_to(player_t *player, int x, int y)
{
  return abs(player->x - x) + abs(player->y - y);
}
