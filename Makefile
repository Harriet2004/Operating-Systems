C = gcc
FLAGS = -Wall -Werror
AIFLAG = -Ipthread

all: mmcopier mscopier
mmcopier: mmcopier.o
	$(C) mmcopier.o -o mmcopier
mscopier: mscopier.o
	$(C) mscopier.o -o mscopier
mmcopier.o: mmcopier.c 
	$(C) $(FLAGS) -c mmcopier.c -o mmcopier.o
mscopier.o: mscopier.c 
	$(C) $(FLAGS) -c mscopier.c -o mscopier.o
clean:
	rm -rf *.o mmcopier mscopier