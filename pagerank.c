#include "xerrori.h"
#include <math.h>
#define QUI __LINE__,__FILE__
#define BufSize 10 

/*per ottimizzare la ricerca di archi entranti in uno specifico nodo 
gli array presenti nel tipo inmap andranno mantenuti sempre ordinati in modo crescente 
in modo da controllare efficentemente se un arco sia gia stato inserito o meno */
typedef struct {
    int *inArrow ; //vettore utilizzato come set contenente nodi 
    int len ; //lunghezza sel vettore inArrow
} inmap ;

typedef struct {
    int N  ; //numero nodi grafo
    int *out ; //array con numero di archi uscenti da ogni nodo
    inmap *in ; //array con gli insiemi di archi entranti in ogni nodo
    pthread_mutex_t *mutex_arr; //mutex utilizzato per coordinare accesso dati del grafo
} grafo ;

typedef struct {
    int i ; //nodo di partenza dell'arco
    int j ; //nodo di arrivo dell'arco
} coppia ;

typedef struct {
    coppia *Buffer ; //buffer
    pthread_mutex_t *mutex_buf ; //mutex accesso al buffer
    sem_t *FreePlace ; //semaforo che indica i posti liberi nel buffer
    sem_t *ItemNumber ; //semaforo che indica gli elementi presenti nel buffer
    int *buffindex ; //indice degli elementi nel buffer
    grafo *g ;
} datiC ; //struttura dati consumatore

typedef struct {
    coppia *Buffer ; //buffer
    sem_t *FreePlace ; //semaforo che indica i posti liberi nel buffer
    sem_t *ItemNumber ; //semaforo che indica gli elementi presenti nel buffer
    int *buffindex ; //indice degli elementi nel buffer
    int ThNumber ; //numero di thread
} datiP; //struttura datu produttore

typedef struct {
    int j ; //nodo del quale abbiamo calcolato il pagerank
    double x ; //nuovo valore del pagerank del nodo j 
    double e ; //nuovo errore del nodo j
} OutBuf ;

typedef struct {
    int *InBuf ; //buffer per la comunicazione da thread principale a thread ausiliario
    int *InBufIndex ; //indice per accesso al buffer in ingresso
    pthread_mutex_t *mutexIn ; //mutex per accesso esclusivo al buffer in entrata
    sem_t *FreePlaceIn ; //semaforo che indica posti liberi nel buffer in entrata
    sem_t *ItemNumberIn ; //semaforo che indica numero di elementi nel buffer in entrata

    OutBuf *Out ; //buffer per la comunicazione da thread ausiliario a thread principale
    int *OutBufIndex ; //indice per accesso al buffer in uscita
    pthread_mutex_t *mutexOut ; //mutex per accesso esclusivo al buffer in uscita  
    sem_t *FreePlaceOut ; //semaforo che indica posti liberi nel buffer in uscita
    sem_t *ItemNumberOut ; //semaforo che indica numero di elementi nel buffer in uscita 
} PageRankBuf ;

typedef struct {
    PageRankBuf buffer ; //buffer per la comunicazione tra thread principale e ausiliari
    double *X ; //vettore contenente il corrente valore del pagerank
    double *Y ; //vettore contenente il corrente valore Y
    double TeleFactor ;
    double *S ;
    double d ;
    grafo *g ;
} PageRankdata ;

//Funzione che cernca un particolare intero all'interno di un array 
bool BinarySearch(int target, int *arr, int len) ;

//Funzione MergeSort
void merge(int *arr, int left, int mid, int right);
void mergeSort(int *arr, int left, int right) ;

//funzione che inizzializza i parametri inseriti dall'utente sulla linea di comando
void ParsingCommandLine(int *NumberOfTopNodes, int *MaxIteration, double *DampFactor, double *MaxError,int *ThreadNumber ,char **FileName,int argc, char*argv[]) ;

//funzione che ritorna il numero di nodi del grafo
int ReadingNumberOfNode(char *FileName) ;

//funzione che legge dal file gli archi e li inserisce sul buffer
void ReadingFile(char *FileName, void *arg ) ;

//thread body che gestisce gli archi
void *ArchManagement(void *arg) ;

//algoritmo per il calcolo del PageRank
double *pagerank(grafo *g, double d, double eps, int maxiter, int taux, int *numiter) ;

//thread body che calcola il pagerank per uno specifico elemento j
void *PagerankCalc(void *arg) ; 

int main(int argc, char *argv[]){
    //controllo che il parametro obbligatorio sia stato inserito
    if(argc < 2){
        printf("Il nome del file è un parametro obbligatorio\n") ;
        printf("Uso: %s nomefile \n",argv[0]) ;
        return 1 ;
    }
    
    //valore standrd dati in input
    int NumberOfTopNodes = 3 ;
    int MaxIteration = 100 ;
    double DampFactor = 0.9 ;
    double MaxError = 1.e-4 ;
    int ThreadNumber = 3 ;
    char *FileName = "" ;

    //dati di sincronizzazione thread
    pthread_t th[ThreadNumber] ;
    sem_t FreePlace , ItemNumber ;
    pthread_mutex_t mutexBuf = PTHREAD_MUTEX_INITIALIZER ;
    pthread_mutex_t mutexArr = PTHREAD_MUTEX_INITIALIZER ;
    coppia buffer[BufSize] ;
    int indexP = 0 , indexC = 0 ;

    //parsing della linea di comando
    ParsingCommandLine(&NumberOfTopNodes,&MaxIteration,&DampFactor,&MaxError,&ThreadNumber,&FileName,argc,argv) ;
    
    //inizzializzazione semafori per la gestione del buffer
    xsem_init(&FreePlace,0,BufSize,QUI) ;
    xsem_init(&ItemNumber,0,0,QUI) ;

    //inizzializzazione struttura dati grafo
    int NodeNumber = ReadingNumberOfNode(FileName) ;
    
    grafo g ;
    g.N = NodeNumber ;
    g.out = calloc(NodeNumber,sizeof(int)) ; 
    g.in = malloc(NodeNumber*sizeof(inmap)) ;
    g.mutex_arr = &mutexArr ;


    //inizzializzazioni campi array di tipo inmap
    for(int i = 0 ; i < NodeNumber ; i++){
        g.in[i].len = 0 ;
        g.in[i].inArrow = malloc((g.in[i].len)*sizeof(int)) ;
    }

    //inizzializzazione struttura dati per thread produttore
    datiP produttore ;
    produttore.Buffer = buffer ;
    produttore.buffindex = &indexP ;
    produttore.FreePlace = &FreePlace ;
    produttore.ItemNumber = &ItemNumber ;
    produttore.ThNumber = ThreadNumber ;

    //inizzializzazione struttura dati per thread consumatore
    datiC consumatore[ThreadNumber] ;
    
    for(int i = 0 ; i <ThreadNumber ; i++){
        consumatore[i].Buffer = buffer ;
        consumatore[i].FreePlace = &FreePlace ;
        consumatore[i].ItemNumber = &ItemNumber ;
        consumatore[i].mutex_buf = &mutexBuf ;
        consumatore[i].buffindex = &indexC ;
        consumatore[i].g = &g ;
        xpthread_create(&th[i],NULL,&ArchManagement,&consumatore[i],QUI) ;
    }
    
    //lettura del file e caricamento del buffer
    ReadingFile(FileName,&produttore) ;
    

    //attendo la fine dei consumatori
    for(int i = 0 ; i < ThreadNumber ; i++){
        xpthread_join(th[i],NULL,QUI) ;
    }

    //distruggo i semafori e mutex che non saranno più utilizzinati
    xsem_destroy(&FreePlace,QUI) ;
    xsem_destroy(&ItemNumber,QUI) ;
    xpthread_mutex_destroy(&mutexBuf,QUI) ;
    xpthread_mutex_destroy(&mutexArr,QUI) ;

    /*
    for(int i = 0 ; i < 9 ; i++){
        printf("numero di archi uscenti del dono %d  : %d\n",i,g.out[i]) ;
        printf("archi entranti nel nodo %d\n",i) ;
        for(int j = 0 ; j < g.in[i].len ; j++){
            printf("%d -> %d\n",g.in[i].inArrow[j],i) ;
        }
        printf("\n") ;
    }*/
    int IterationNumber = 0 ;
    double *risultato = pagerank(&g,DampFactor,MaxError,MaxIteration,ThreadNumber,&IterationNumber) ;
    printf("Fine Pagerank\n") ;
    //dealloco gli elementi del grafo
    free(g.out) ;
    for(int i = 0 ; i < NodeNumber ; i++){
        free(g.in[i].inArrow) ; 
    }
    free(g.in) ;

    //dealloco il vettore risultato
    free(risultato) ;
    
    return 0 ;
}

bool BinarySearch(int target, int *arr, int len){
    int left = 0 , right = len-1 , mid ;

    while(left<=right){
        mid = left + (right-left)/2 ;

        if(arr[mid] == target){
            return true ;
        }else{
            if(arr[mid] > target){
                right = mid - 1 ;
            }else{
                left = mid + 1 ;
            }
        }
    }
    return false ;
}

void merge(int *arr, int left, int mid, int right) {
    int i, j, k;
    int n1 = mid - left + 1;
    int n2 = right - mid;

    
    int L[n1], R[n2];

    
    for (i = 0; i < n1; i++)
        L[i] = arr[left + i];
    for (j = 0; j < n2; j++)
        R[j] = arr[mid + 1 + j];

    // unione dei due array
    i = 0;
    j = 0;
    k = left;

    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) {
            arr[k] = L[i];
            i++;
        } else {
            arr[k] = R[j];
            j++;
        }
        k++;
    }

    // Copio gli elementi rimanenti di entrambi i vettori
    while (i < n1) {
        arr[k] = L[i];
        i++;
        k++;
    }

    
    while (j < n2) {
        arr[k] = R[j];
        j++;
        k++;
    }
}

void mergeSort(int *arr, int left, int right) {
    if (left < right) {
        int mid = left + (right - left) / 2;

        mergeSort(arr, left, mid);
        mergeSort(arr, mid + 1, right);

        // unione dei due array 
        merge(arr, left, mid, right);
    }
}

void ParsingCommandLine(int *NumberOfTopNodes, int *MaxIteration, double *DampFactor, double *MaxError, int *ThreadNumber , char **FileName, int argc, char*argv[]){
    int option ;
    //utilizzo del funzione getopt per il parsing della linea di comando
    while((option = getopt(argc,argv,"k:d:m:e:t:")) != -1){
        switch(option){
            case 'k' :
                if(optarg != NULL){
                    (*NumberOfTopNodes) = atoi(optarg) ;
                }else{
                    termina("Errore nel parsing TopNodes") ;
                }
            break ;

            case 'm' :
                if(optarg != NULL){
                    (*MaxIteration) = atoi(optarg) ;
                }else{
                    termina("Errore nel parsing MaxIteration") ;
                }
            break ;

            case 'd' :
                if(optarg != NULL){
                    (*DampFactor) = atof(optarg) ;
                }else{
                    termina("Errore nel parsing DampFactor") ;
                }
            break ;

            case 'e' :
                if(optarg != NULL){
                    (*MaxError) = atof(optarg) ;
                }else{
                    termina("Errore nel parsing MaxError") ;
                }
            break ;

            case 't' :
                if(optarg != NULL){
                    (*ThreadNumber) = atoi(optarg) ;
                }else{
                    termina("Errore nel parsing ThreadNumber") ;
                }
            break; 

        }
    }

    (*FileName) = argv[argc -1] ;
}

int ReadingNumberOfNode(char *FileName){
    FILE *f  = fopen(FileName,"r");
    //dati che rappresentano il numero di righe, colonne della matrice di adiacienza e il numero totale di archi 
    int r = 0, c = 0, n = 0 ;

    char *line = "";
    size_t len = 0 ;
    int nread = 0 ;

    if(f == NULL){
        termina("Errore apertura file") ;
    }

    /*per il momento mi interessa solo la lettura del numero di nodi 
    necessario per l'inizzializzazione della struttura dati del grafo*/
    while((nread = getline(&line,&len,f)) != -1){
        if(line[0] != '%'){
            r = atoi(&line[0]) ;
            c = atoi(&line[2]) ;
            n = atoi(&line[4]) ;

            break ;
        }
    }

    //controllo che il numero di righe colonne e nodi sia coerente
    assert(r > 0 && c >0) ;
    assert(n>0) ;
    assert(r==c) ;

    free(line) ;
    fclose(f) ;
    return r ;
}

void ReadingFile(char *FileName, void *arg ){
    //casting della struttura dati
    datiP *d = (datiP *)arg ;

    FILE *f  = fopen(FileName,"r");

    //dati che rappresentano il numero di righe, colonne della matrice di adiacienza e il numero totale di archi 
    int r, c, n ;
    bool DatiIniziali = true ;

    //dati che rappresentano nodo entrante e uscente di un arco
    int i, j ;

    //dati necessari per l'utilizzo della funzione getline
    char *line ="";
    size_t len = 0 ;
    int nread ;

    if(f == NULL){
        termina("Errore apertura file") ;
    }

    while((nread = getline(&line,&len,f)) != -1){
       if(line[0] != '%'){
            if(DatiIniziali){
                r = atoi(&line[0]) ;
                c = atoi(&line[2]) ;
                n = atoi(&line[4]) ;

                //controllo che il numero di righe colonne e nodi sia coerente
                assert(r > 0 && c >0) ;
                assert(n>0) ;
                assert(r==c) ;

                DatiIniziali = false ;
            }else{
                i = atoi(&line[0]) ;
                j = atoi(&line[1]) ;

                //controlli sul valore dei nodi
                assert(1 <= i) ;
                assert(i <= r) ;

                assert(1 <= j) ;
                assert(j <= c) ;

                //inserimento dei nodi all'interno del buffer ;

                xsem_wait(d->FreePlace,QUI) ;
                d->Buffer[*(d->buffindex) % BufSize].i = i ;
                d->Buffer[*(d->buffindex) % BufSize].j = j ;
                
                (*d->buffindex) += 1 ;
                xsem_post(d->ItemNumber,QUI) ;
            }
       }
    }

    //comunico ai consumatori la fine della lettura del file 
    for(int i = 0 ; i < d->ThNumber; i++){
        xsem_wait(d->FreePlace,QUI) ;
        d->Buffer[*(d->buffindex) % BufSize].i = -1 ; 
        d->Buffer[*(d->buffindex) % BufSize].j = -1 ; 
        (*d->buffindex) +=1 ;
        xsem_post(d->ItemNumber,QUI) ;
    }
    
    free(line) ;
    fclose(f) ;
    
}

void *ArchManagement(void *arg){
    datiC *d = (datiC *)arg ;
    int i , j ;

    while(true){
        //prelevo i e j dal buffer utilizzando un mutex per l'accesso esclusivo
        xpthread_mutex_lock(d->mutex_buf,QUI) ;
        xsem_wait(d->ItemNumber,QUI) ;

        i = d->Buffer[*(d->buffindex) % BufSize].i ;
        j = d->Buffer[*(d->buffindex) % BufSize].j ;
        
        (*d->buffindex) += 1 ;

        xsem_post(d->FreePlace,QUI) ;
        xpthread_mutex_unlock(d->mutex_buf,QUI) ;
        
        //se entrambi i e j sono uguali a -1 ho finito la lettura 
        if(i == -1 && j == -1){
            break ;
        }

        if(i == j) continue; 
        
        i -= 1 ;
        j -= 1 ;
        
        
        //utilizzo un mutex per l'accesso esclusivo ai vettori in e out
        xpthread_mutex_lock(d->g->mutex_arr,QUI) ;

        //essendo il vettore in ordinato faccio una ricerca binaria per vedere se un valore è presente al suo interno
        if(!BinarySearch(i,d->g->in[j].inArrow,d->g->in[j].len)){

            //inserimento dell'elemento i allinterno dell'array di archi entranti in j 
            d->g->in[j].len += 1 ;
            d->g->in[j].inArrow = realloc(d->g->in[j].inArrow,d->g->in[j].len*sizeof(int)) ;
            d->g->in[j].inArrow[d->g->in[j].len-1] = i ;

            //ordinamento dell'array
            mergeSort(d->g->in[j].inArrow,0,d->g->in[j].len-1) ;
            
            //aumento dal valore degli archi uscenti dal nodo i
            d->g->out[i] += 1 ;
        }
        xpthread_mutex_unlock(d->g->mutex_arr,QUI) ;
    }
    
    pthread_exit(NULL) ;
}

double *pagerank(grafo *g, double d, double eps, int maxiter, int taux, int *numiter){
    //inizzializzazione vettori
    double *X = malloc(g->N*sizeof(double)) ;
    double *Y = malloc(g->N*sizeof(double)) ;
    double *NewX = malloc(g->N*sizeof(double)) ; 

    //inizzializzazione elementi per la sincronizzazione
    sem_t FreePlaceIn , FreePlaceOut ;
    sem_t ItemNumberIn , ItemNumberOut ;
    pthread_mutex_t mutexIn  = PTHREAD_MUTEX_INITIALIZER ;
    pthread_mutex_t mutexOut = PTHREAD_MUTEX_INITIALIZER ;

    xsem_init(&FreePlaceIn,0,BufSize,QUI) ;
    xsem_init(&FreePlaceOut,0,BufSize,QUI) ;
    xsem_init(&ItemNumberIn,0,0,QUI) ;
    xsem_init(&ItemNumberOut,0,0,QUI) ;

    int InBufIndex = 0, OutBufIndex = 0 , InBufMain = 0;

    //inizzializzazione buffer
    PageRankBuf buffer ;

    buffer.InBuf = malloc(BufSize * sizeof(int)) ;
    buffer.InBufIndex = &InBufIndex ;
    buffer.FreePlaceIn = &FreePlaceIn ;
    buffer.ItemNumberIn = &ItemNumberIn ;
    buffer.mutexIn = &mutexIn ;

    buffer.Out = malloc(BufSize*sizeof(OutBuf)) ;
    buffer.OutBufIndex = &OutBufIndex ;
    buffer.FreePlaceOut = &FreePlaceOut ;
    buffer.ItemNumberOut = &ItemNumberOut ;
    buffer.mutexOut = &mutexOut ;

    //inizzializzazione valori necessari al calcolo del Pagerank    
    double error = 1 ;
    double TeleportFactor = (1-d)/g->N ;

    int *DeadNodes = malloc(g->N*sizeof(int)) ;
    int NumberOfDeadNodes = 0 ;

    int *NotDeadNodes = malloc(g->N*sizeof(int)) ;
    int NumberdOfNotDeadNodes = 0 ;

    //inizzializzazione del vettore pagerank e ricerca nodi senza archi uscenti
    for(int i = 0 ; i < g->N ; i++){
        
        X[i] = 1.0/g->N ;

        if(g->out[i] == 0){
            DeadNodes[NumberOfDeadNodes] = i ;
            NumberOfDeadNodes += 1 ;
        }else{
            NotDeadNodes[NumberdOfNotDeadNodes] = i ;
            NumberdOfNotDeadNodes += 1 ;
        }
    }

    DeadNodes = realloc(DeadNodes,NumberOfDeadNodes*sizeof(int)) ;
    NotDeadNodes = realloc(NotDeadNodes,NumberdOfNotDeadNodes*sizeof(int)) ;

    if(DeadNodes == NULL) termina("Errore reallocazione vettore DeadNodes") ;
    if(NotDeadNodes == NULL) termina("Errore reallocazione vettore NotDeadNodes") ;
    double S = 0.0 ;

    //partenza thread
    pthread_t th[taux] ;
    PageRankdata data[taux] ;

    for(int i = 0 ; i < taux ; i++){
        data[i].buffer = buffer ;
        data[i].g = g ;
        data[i].X = X ;
        data[i].Y = Y ;
        data[i].TeleFactor = TeleportFactor ;
        data[i].S = &S ;
        data[i].d = d ;
        xpthread_create(&th[i],NULL,&PagerankCalc,&data[i],QUI) ;
    }

    while(error > eps && (*numiter) < maxiter ){
        error = 0 ;
        //calcolo contributi DeadNodes
        for(int i = 0 ; i < NumberOfDeadNodes ; i++){
            S += X[DeadNodes[i]] ;
        }
        
        S = (d/g->N) * S ;
        
        //calcolo vettore Y 
        for(int i = 0 ; i < NumberdOfNotDeadNodes ; i++){
            Y[NotDeadNodes[i]] = X[NotDeadNodes[i]]/g->out[NotDeadNodes[i]] ;
        }

        //caricamento buffer in entrata verso i thread ausiliari
        //ogni thread si dovra occupare del calcolo della i-esima componente del vettore
        for(int i = 0 ; i < g->N ; i++){
            xsem_wait(&FreePlaceIn,QUI) ;
            buffer.InBuf[InBufMain%BufSize] = i ;
            xsem_post(&ItemNumberIn,QUI) ;

            InBufMain += 1 ;
        }

        //estrazione dal buffer di uscita dei nouvi valori calcolati
        for(int i = 0 ; i < g->N ; i++){
            xsem_wait(&ItemNumberOut,QUI) ;

            NewX[buffer.Out[i].j] = buffer.Out[i%BufSize].x ;
            error += fabs((buffer.Out[i%BufSize].e)) ;

            xsem_post(&FreePlaceIn,QUI) ;
        }

        //aggiornamento valori del vettore X per inizio nuova iterazione
        for(int i = 0 ; i < g->N ; i++){
            X[i] = NewX[i] ;
        }

        (*numiter) += 1 ;
        //re inizzializzo i valori del semaforo per il buffer in uscita 
        xsem_init(&FreePlaceOut,0,BufSize,QUI) ;
        xsem_init(&ItemNumberOut,0,0,QUI) ;

    }
    //mando segnali ai thread di terminare
    for(int i = 0 ; i < taux ; i++){
        xsem_wait(&FreePlaceIn,QUI) ;
        buffer.InBuf[InBufMain%BufSize] = -1 ;
        xsem_post(&ItemNumberIn,QUI) ;

        InBufMain += 1 ;
    }

    //aspetto la fine dei thread ausiliari
    for(int i = 0 ; i < taux ; i++){
        xpthread_join(th[i],NULL,QUI) ;
    }

    free(X) ;
    free(Y) ;
    free(NotDeadNodes) ;
    free(DeadNodes) ;
    free(buffer.InBuf) ;
    free(buffer.Out) ;
    return NewX ;
}

void *PagerankCalc(void *arg){
    PageRankdata *data = (PageRankdata *) arg ;
    int j = 0 ;
    float somma  = 0.0, newx = 0.0 , newe = 0.0;

    while(true){
        somma = 0 ;
        newx = 0 ;
        newe = 0 ;

        //prelevo l'indice di cui dovremmo calcolare il pagerank dal buffer
        
        xsem_wait(data->buffer.ItemNumberIn,QUI) ;
        xpthread_mutex_lock(data->buffer.mutexIn,QUI) ;
        
        j = data->buffer.InBuf[(*data->buffer.InBufIndex)%BufSize] ;
        (*data->buffer.InBufIndex) += 1 ;

        xpthread_mutex_unlock(data->buffer.mutexIn,QUI) ;
        xsem_post(data->buffer.FreePlaceIn,QUI) ;
        
        if(j == -1) pthread_exit(NULL) ;
        //calcolo del nuovo fattore pagerank
        for(int i = 0 ; i < data->g->in[j].len ; i++){
            somma += data->Y[data->g->in[j].inArrow[i]] ;
        }

        newx += data->TeleFactor + (*data->S) + data->d * somma ;
        newe = fabs(data->X[j] -newx ) ;

        //caricamento del nuovo pagerank sul buffer in uscita
        
        xsem_wait(data->buffer.FreePlaceOut,QUI) ;
        xpthread_mutex_lock(data->buffer.mutexOut,QUI) ;
        
        data->buffer.Out[(*data->buffer.OutBufIndex)%BufSize].j = j ;
        data->buffer.Out[(*data->buffer.OutBufIndex)%BufSize].x = newx ;
        data->buffer.Out[(*data->buffer.OutBufIndex)%BufSize].e = newe ;
        (*data->buffer.OutBufIndex) += 1 ;
        
        xpthread_mutex_unlock(data->buffer.mutexOut,QUI) ;
        xsem_post(data->buffer.ItemNumberOut,QUI) ;
       
    }
}