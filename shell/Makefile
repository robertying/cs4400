RT = run_tournament.c

CC = gcc
CFLAGS = -Wall -O2 -I.

HELPERS = io.c
HELPERS_DEP = $(HELPERS) io.h

PLAYERS = scan_player step_player jump_player fumble_player garbled_player distracted_player unreliable_player 

all: game_maker $(PLAYERS) run_tournament

game_maker: game_maker.c io.c random.c io.h random.h
	$(CC) $(CFLAGS) -o game_maker game_maker.c io.c random.c

run_tournament: $(RT) csapp.c csapp.h
	$(CC) $(CFLAGS) -o run_tournament $(RT) csapp.c

# Generic rule for players:
%: %.c io.c io.h
	$(CC) $(CFLAGS) -o $@ $< io.c

clean:
	rm -f run_tournament game_maker $(PLAYERS)
