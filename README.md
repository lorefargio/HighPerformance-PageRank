Nell'implementazionde dell'algoritmo per il calcolo del PageRank i threads vengono utilizzati :

Per la gestione degli archi una volta letti dal file, utilizzando un classico paradigma produttore consumatore, Il thread principale legge la coppia di nodi dal file e la inserisce su un buffer, i thread ausiliari prendono la coppia e la utilizzano, dopo gli opportuni controlli, per inizzializzare la struttura dati del grafo.

Per quanto riguarda invece il vero e proprio calcolo del vettore PageRank:
Il main thread si occupa dei calcoli che vanno eseguiti una sola volta per iterazione, come il calcolo del vettore Y, o di fattori che non dipendono ne dal nodo sul quale vogliamo calcolare il pagerank o dal numero di iterazione.

Il thread principale passa ai thread utilizzati per il calcolo una struttura dati contenente, un puntatore ad una variabile intera che viene utilizzata come indice j sul quale effettuare il calcolo, un mutex e due ConditionVariable che vengono utilizzate per la sincronizzazione dei vari thread.

I thread ausiliari si occupano del calcolo della nuova componente Xj e del suo errore relativo alla vecchia componente, nel mentre il thread principale attende facendo una wait sulla conditon variables "IndiceRaggiunto".

Nel momento in cui l'indice che viene utilizzato per sapere di quale componente Xj dobbiamo effettuare il calcolo non è piu valido, i thread ausiliari utilizzano una signal sulla ConditionVariables "IndiceRaggiunto" per indicare al thread prinicipale il raggiungimento del valore massimo, in seguito si mettono ad aspettare con una wait sull'altra variabile "IndiceResettato". 

Il main thread una volta recepito il segnale da parte di uno dei thread ausiliari, copia i nuovi valori appena calcolati nel vettore X utilizzato per il calcolo dei valori nell'iterazione successiva, controlla le condizioni di uscita e in caso non siano soddisfatte, calcola il nuovo parametro S e il nuovo vettore Y, dopo di che resetta l'indice e fa una broadcast in modo da risvegliare i thread adibiti al calcolo delle componenti del vettore PageRank.
 
