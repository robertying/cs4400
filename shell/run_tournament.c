#include <stdio.h>
#include <stdlib.h>
#include "csapp.h"

typedef struct result
{
  int win;
  int lose;
  int fail;
} result;

static result *results = NULL;
static int player_num = 0;
static int current_round = 0;

static void run_tournament(int seed, int rounds, int num_progs, char **progs);

int main(int argc, char **argv)
{
  int rounds, seed;

  if (argc < 4)
  {
    fprintf(stderr, "%s: expects <random-seed> <round-count> <player-program> <player-program> ...\n", argv[0]);
    return 1;
  }

  seed = atoi(argv[1]);
  if (seed < 1)
  {
    fprintf(stderr, "%s: bad random seed; not a positive number: %s\n", argv[0], argv[1]);
    return 1;
  }

  rounds = atoi(argv[2]);
  if (rounds < 1)
  {
    fprintf(stderr, "%s: bad round count; not a positive number: %s\n", argv[0], argv[2]);
    return 1;
  }

  player_num = argc - 3;
  results = Calloc(player_num, sizeof(result));

  run_tournament(seed, rounds, argc - 3, argv + 3);

  Free(results);

  return 0;
}

static void report()
{
  char c_round[sizeof(int) * 4 + 1];
  sprintf(c_round, "%d\n", current_round + 1);
  Sio_puts(c_round);

  int i;
  for (i = 0; i < player_num; i++)
  {
    char c_result[sizeof(int) * 4 + 1];
    sprintf(c_result, "%d %d %d\n", results[i].win, results[i].lose, results[i].fail);
    Sio_puts(c_result);
  }

  exit(0);
}

static void run_tournament(int seed, int rounds, int n, char **progs)
{
  // signal settings
  Signal(SIGPIPE, SIG_IGN);
  sigset_t sigs;
  Sigemptyset(&sigs);
  Sigaddset(&sigs, SIGINT);
  Signal(SIGINT, report);

  // initialize pipes
  int in_game[2];
  int out_game[2];
  int **in_player = malloc(n * sizeof(int *));
  int **out_player = malloc(n * sizeof(int *));
  int i;
  for (i = 0; i < n; ++i)
  {
    in_player[i] = malloc(2 * sizeof(int));
    out_player[i] = malloc(2 * sizeof(int));
  }

  // initialize
  pid_t *pids = malloc(n * sizeof(pid_t));
  // 0: normal
  // 1: fail
  char *fail = Calloc(n, sizeof(char));

  for (current_round = 0; current_round < rounds; ++current_round)
  {
    // block until the round finishes
    Sigprocmask(SIG_BLOCK, &sigs, NULL);

    // run game_maker
    Pipe(in_game);
    Pipe(out_game);
    int maker_pid = Fork();
    if (maker_pid == 0)
    {
      char c_seed[sizeof(int) * 4 + 1];
      sprintf(c_seed, "%d", seed + current_round);
      char c_n[sizeof(int) * 4 + 1];
      sprintf(c_n, "%d", n);
      char *args[] = {"game_maker", c_seed, c_n, "40", "45", NULL};

      Dup2(in_game[0], 0);
      Dup2(out_game[1], 1);
      Setpgid(0, 0);
      Execve(args[0], args, environ);
    }

    // run players
    for (i = 0; i < n; i++)
    {
      Pipe(in_player[i]);
      Pipe(out_player[i]);

      pids[i] = Fork();
      if (pids[i] == 0)
      {
        Dup2(in_player[i][0], 0);
        Dup2(out_player[i][1], 1);
        Close(out_player[i][1]);
        Setpgid(0, 0);

        char *args[] = {progs[i], NULL};
        Execve(args[0], args, environ);
      }
      Close(out_player[i][1]);
    }

    // get initial positions
    char target[7];
    Rio_readn(out_game[0], target, 7);
    for (i = 0; i < n; ++i)
    {
      char start[7];
      Rio_readn(out_game[0], start, 7);
      Rio_writen(in_player[i][1], start, 7);
    }

    memset(fail, 0, n * sizeof(int));
    int fail_count = 0;
    int winner_idx = -1;
    int player = 0;
    // take turns
    while (fail_count != n)
    {
      if (fail[player] == 0)
      {
        char buffer[7];
        int read = Read(out_player[player][0], buffer, 7);
        //printf("next move: %s", buffer);
        if (read == 7)
        {
          Rio_writen(in_game[1], buffer, 7);
        }
        else
        {
          // player partial output
          Rio_writen(in_game[1], "broken\n", 7);
        }
        Rio_readn(out_game[0], buffer, 7);
        //printf("game response: %s", buffer);
        if (!(strcmp(buffer, "wrong!\n")))
        {
          fail[player] = 1;
          fail_count++;
        }
        else
        {
          Rio_writen(in_player[player][1], buffer, 7);
        }
        if (!(strcmp(buffer, "winner\n")))
        {
          winner_idx = player;
          break;
        }
      }

      player++;
      if (player == n)
      {
        player = 0;
      }
    }

    // check winner status
    if (winner_idx != -1)
    {
      results[winner_idx].win++;

      int exit_status;
      Waitpid(pids[winner_idx], &exit_status, 0);
      if (WIFEXITED(exit_status) && WEXITSTATUS(exit_status) != 0)
      {
        fail[winner_idx] = 1;
      }
      if (WIFSIGNALED(exit_status) && WTERMSIG(exit_status) != 0)
      {
        fail[winner_idx] = 1;
      }
    }

    // round complete
    for (i = 0; i < n; ++i)
    {
      if (fail[i] == 1)
      {
        results[i].fail++;
      }
      if (i != winner_idx)
      {
        results[i].lose++;
        Kill(pids[i], SIGKILL);
        Waitpid(pids[i], NULL, 0);
      }
    }
    Waitpid(maker_pid, NULL, 0);

    // clean
    Close(in_game[1]);
    Close(in_game[0]);
    Close(out_game[1]);
    Close(out_game[0]);
    for (i = 0; i < n; ++i)
    {
      Close(in_player[i][0]);
      Close(in_player[i][1]);
      Close(out_player[i][0]);
    }

    //unblock to report
    Sigprocmask(SIG_UNBLOCK, &sigs, NULL);
  }

  // print results
  printf("%d\n", rounds);
  for (i = 0; i < n; i++)
  {
    printf("%d %d %d\n", results[i].win, results[i].lose, results[i].fail);
  }

  // clean
  Free(pids);
  Free(fail);
  for (i = 0; i < n; ++i)
  {
    Free(in_player[i]);
    Free(out_player[i]);
  }
  Free(in_player);
  Free(out_player);
}
