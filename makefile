# definizione del compilatore e dei flag di compilazione
# che vengono usate dalle regole implicite
CC=gcc
CFLAGS=-std=c11 -Wall -g -O -pthread
LDLIBS=-lm -lrt -pthread

EXECS = pagerank

all: $(EXECS)

#creazione file eseguibili utilizzando xerrori.o
pagerank: pagerank.o xerrori.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

#creazione file oggetto dipendenti da xerrori.h
pagerank.o: pagerank.c xerrori.h
	$(CC) $(CFLAGS) -c $<

#cancellazione dei file oggetto e degli eseguibili
clean: 
	rm -f *.o $(EXECS)