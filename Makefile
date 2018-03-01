Target = mytar
CC = gcc
CFLAGS = -pedantic -std=gnu99 -Wall -g
MAIN = mytar
OBJS = mytar.o

all : $(MAIN)

$(MAIN) : $(OBJS)
	$(CC) -o $(MAIN) $(CFLAGS) mytar.c

mytar.o : mytar.c
	$(CC) $(CFLAGS) -c mytar.c

clean :
	rm *.o $(MAIN) core
