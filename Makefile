CC=gcc
CFLAGS=-I.

hw2: hw2.o
	$(CC) -o hw2 hw2.o -I.

clean:
	rm -f *.o 
