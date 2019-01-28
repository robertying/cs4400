CC = gcc
CFLAGS = -Wall -O2
LIBS = -lm

OBJS = driver.o kernels.o fcyc.o clock.o
KERNELS = kernels.c

all: driver

driver: $(OBJS) config.h defs.h fcyc.h
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o driver

kernels.o: $(KERNELS)
	$(CC) $(CFLAGS) -I. -o kernels.o -c $(KERNELS)

clean: 
	-rm -f $(OBJS) driver core *~ *.o
