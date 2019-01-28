#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "io.h"

void write_position(int x, int y)
{
  printf("%2.2d, %2.2d\n", x, y);
  fflush(stdout);
}

static int parse_a_number(char *buffer, int *n, int is_dist)
{
  if ((buffer[0] < '0') || (buffer[0] > '9'))
    return 0;
  if ((buffer[1] < '0') || (buffer[1] > '9'))
    return 0;
  *n = (buffer[0] - '0') * 10 + (buffer[1] - '0');
  return 1;
}

int parse_position(char *buffer, int *x, int *y)
{
  if (!parse_a_number(buffer, x, 0))
    return 0;

  if (buffer[2] != ',')
    return 0;
  if (buffer[3] != ' ')
    return 0;

  if (!parse_a_number(buffer + 4, y, 0))
    return 0;

  if (buffer[6] != '\n')
    return 0;

  return 1;
}

void read_position(const char *who, int *x, int *y)
{
  char buffer[7];

  fread(buffer, 1, 7, stdin);
  if (!parse_position(buffer, x, y)) {
    fprintf(stderr, "%s: bad position\n", who);
    exit(1);
  }
}

int read_guidance(const char *who)
{
  char buffer[8];
  int n;

  n = fread(buffer, 1, 7, stdin);
  buffer[n] = 0;
  if (!strcmp(buffer, "hotter\n"))
    return HOTTER; 
  else if (!strcmp(buffer, "colder\n"))
    return COLDER;
  else if (!strcmp(buffer, "steady\n"))
    return STEADY;
  else if (!strcmp(buffer, "winner\n"))
    return WINNER;

  fprintf(stderr, "%s: bad guidance %s\n", who, buffer);
  exit(1);
}
