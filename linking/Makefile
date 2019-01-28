ENFORCE = enforce.c
RACKET = racket

CC = gcc
CFLAGS = -Wall -O2

TEST_INPUTS_80 = ex0.so
TEST_INPUTS_90 = $(TEST_INPUTS_80) ex1.so
TEST_INPUTS_100 = $(TEST_INPUTS_90) ex2.so ex3.so ex4.so
TEST_INPUTS = $(TEST_INPUTS_100)

all: enforce call $(TEST_INPUTS)

enforce: $(ENFORCE) decode.c decode.h
	$(CC) $(CFLAGS) -o enforce $(ENFORCE) decode.c

call: call.c
	$(CC) $(CFLAGS) -o call call.c -ldl

%.so: %.c it.so
	$(CC) $(CFLAGS) -shared -fPIC -o $@ $< ./it.so

it.so: it.c
	$(CC) $(CFLAGS) -shared -fPIC -o it.so it.c

test80:
	$(MAKE) test TEST_INPUTS="$(TEST_INPUTS_80)"

test90:
	$(MAKE) test TEST_INPUTS="$(TEST_INPUTS_90)"

test100:
	$(MAKE) test TEST_INPUTS="$(TEST_INPUTS_100)"

test: enforce call $(TEST_INPUTS)
	for i in $(TEST_INPUTS) ; do  \
          $(RACKET) test.rkt $$i; \
        done

clean:
	rm -f enforce call *.so
