Nell'implementazionde dell'algoritmo per il calcolo del PageRank i threads vengono utilizzati inizzialmente
per la gestione degli archi una volta letti dal file, utilizzando un classico paradigma produttore consumatore.
Il thread prinicipale legge la coppia di nodi dal file e la inserisce su un buffer, i thread ausiliari prendono la coppia e la utilizzano, dopo gli opportuni controlli per inizzializzare la struttura dati del grafo.

Per quanto riguarda invece il vero e proprio calcolo del vettore PageRank:
Il main thread si occupa dei calcoli che vanno eseguiti una sola volta per iterazione, come il calcolo del vettore Y o di fattori che non dipendono ne dal nodo sul quale vogliamo calcolare il pagerank o dal numero di iterazione.

Il thread principale passa ai thread utilizzati per il calcolo una struttura dati contenente, oltre alle variabili su cui effettuare i calcoli, un puntatore ad una variabile intera che viene utilizzata come indice j su cui effettuare il calcolo, un mutex e due ConditionVariable che vengono utilizzate per la sincronizzazione dei vari thread.

I thread accedono alla variabile a cui "punta" il puntatore e la utilizza appunto come indice j su cui lavorare, nel momento in cui l'indice non è piu valido, i thread ausiliari utilizzano una signal sulla ConditionVariables "IndiceRaggiunto" per indicare al thread prinicipale il raggiungimento del valore massimo, in seguito si mettono ad aspettare con una wait sull'altra variabile "IndiceResettato". 

Il main thread una volta recepito il segnale da parte di uno dei thread ausiliari, copia i nuovi valori appena calcolati nel vettore X utilizzato per il calcolo dei valori nell'iterazione successiva, calcola il nuovo parametro S e il nuovo vettore Y, dopo di che resetta l'indice e fa una broadcast in modo da risvegliare i thread adibiti al calcolo delle componenti del vettore PageRank.
 