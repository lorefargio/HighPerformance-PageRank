# definizione del compilatore e dei flag di compilazione
# che vengono usate dalle regole implicite
CC=gcc
CFLAGS=-std=c11 -Wall -g -O -pthread
LDLIBS=-lm -lrt -pthread

EXECS = pagerank.out 

all: $(EXECS)

#creazione file eseguibili utilizzando xerrori.o
%.out: %.o xerrori.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

#creazione file oggetto dipendenti da xerrori.h
%.o: %.c xerrori.h
	$(CC) $(CFLAGS) -c $<

#cancellazione dei file oggetto e degli eseguibili
clean: 
	rm -f *.o $(EXECS)