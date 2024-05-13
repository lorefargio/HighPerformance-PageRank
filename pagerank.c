#include "xerrori.h"
#include <math.h>
#define QUI __LINE__,__FILE__
#define BufSize 10 

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
    double *X ; //vettore contenente il corrente valore del pagerank
    double *NewX ; //vettore dei nuovi pagerank appena calcolati
    double *Y ; //vettore contenente il corrente valore Y
    double *e ; //errore 
    double TeleFactor ;
    double *S ;
    double d ;
    int *WorkingIndex ;
    pthread_mutex_t *mutex ;
    pthread_cond_t *IndiceRaggiunto ;
    pthread_cond_t *IndiceResettato ;
    grafo *g ;
} PageRankdata ;

typedef struct {
    double value ;
    int index ;
} TopElement ;

//Funzione che cerca un particolare intero all'interno di un array 
bool Search(int target, int *arr, int len) ;

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

//funzione per l'ordinamento con qsort
int comparazione_decrescente(const void *a, const void *b) ;

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
    double MaxError = 1.e-7 ;
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

    
    int IterationNumber = 0 , DeadNodesNumber = 0 , ValidArch = 0 ;
    double RankSum = 0.0;
    
    double *risultato = pagerank(&g,DampFactor,MaxError,MaxIteration,ThreadNumber,&IterationNumber) ;
    TopElement *risordinato = malloc(NodeNumber*sizeof(TopElement)) ;

    for(int i = 0 ; i < NodeNumber ; i++){
        if(g.out[i] == 0){
            DeadNodesNumber += 1 ;
        }
        ValidArch += g.in[i].len ;
        RankSum += risultato[i] ;
        risordinato[i].value = risultato[i] ;
        risordinato[i].index = i ;
    }
    qsort(risordinato,NodeNumber,sizeof(TopElement),comparazione_decrescente) ;
    //stampa risultati
    printf("Number of nodes: %d\n",NodeNumber) ;
    printf("Number od dead-end nodes: %d\n",DeadNodesNumber) ;
    printf("Number of Valid arcs : %d\n",ValidArch) ;
    if(IterationNumber < MaxIteration){
        printf("Converged after %d iterations\n",IterationNumber) ;
    }else{
        printf("Did not converge after %d iterations\n",MaxIteration) ;
    }
    printf("Sum of ranks : %.4f (should be 1)\n",RankSum) ;
    printf("Top %d nodes : \n",NumberOfTopNodes) ;
    for(int i = 0 ; i < NumberOfTopNodes ; i++){
        printf("%d %f\n",risordinato[i].index,risordinato[i].value) ;
    }

    //dealloco gli elementi del grafo
    free(g.out) ;
    for(int i = 0 ; i < NodeNumber ; i++){
        free(g.in[i].inArrow) ; 
    }
    free(g.in) ;

    //dealloco il vettore risultato
    free(risultato) ;
    free(risordinato) ;
    return 0 ;
}

bool Search(int target, int *arr, int len){
    for(int i = 0 ; i < len ; i++){
        if(arr[i] == target) return true ;
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
         if (!(sscanf(line, "%d %d %d", &r, &c, &n) == 3)) {
            termina("Errore lettura info Grafo") ;
         } 
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
                if (!(sscanf(line, "%d %d %d", &r, &c, &n) == 3)) {
                    termina("Errore lettura info Grafo") ;
                } 

                //controllo che il numero di righe colonne e nodi sia coerente
                assert(r > 0 && c >0) ;
                assert(n>0) ;
                assert(r==c) ;

                DatiIniziali = false ;
            }else{
                if(!(sscanf(line,"%d %d",&i,&j) == 2)){
                    termina("Errore lettura archi") ;
                }

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
        if(!Search(i,d->g->in[j].inArrow,d->g->in[j].len)){

            //inserimento dell'elemento i allinterno dell'array di archi entranti in j 
            d->g->in[j].len += 1 ;
            d->g->in[j].inArrow = realloc(d->g->in[j].inArrow,d->g->in[j].len*sizeof(int)) ;
            d->g->in[j].inArrow[d->g->in[j].len-1] = i ;
            
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
    
    pthread_mutex_t mutexWorkIndex  = PTHREAD_MUTEX_INITIALIZER ;
    pthread_cond_t IndiceResettato= PTHREAD_COND_INITIALIZER ;
    pthread_cond_t IndiceRaggiunto= PTHREAD_COND_INITIALIZER ;

    //indice su cui i thread ausiliari andranno a lavorare
    int WorkIndex = 0 ;

    //inizzializzazione valori necessari al calcolo del Pagerank    
    double error = 0 ;
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

    //calcolo contributi DeadNodes
    for(int i = 0 ; i < NumberOfDeadNodes ; i++){
            S += X[DeadNodes[i]] ;
    }
            
    S = (d/g->N) * S ;
            
    //calcolo vettore Y 
    for(int i = 0 ; i < NumberdOfNotDeadNodes ; i++){
            Y[NotDeadNodes[i]] = X[NotDeadNodes[i]]/g->out[NotDeadNodes[i]] ;
    }

    for(int i = 0 ; i < taux ; i++){
        data[i].g = g ;
        data[i].X = X ;
        data[i].NewX = NewX ;
        data[i].Y = Y ;
        data[i].TeleFactor = TeleportFactor ;
        data[i].S = &S ;
        data[i].d = d ;
        data[i].e = &error ;
        data[i].WorkingIndex = &WorkIndex ;
        data[i].mutex = &mutexWorkIndex ;
        data[i].IndiceRaggiunto = &IndiceRaggiunto ;
        data[i].IndiceResettato = &IndiceResettato ;
        xpthread_create(&th[i],NULL,&PagerankCalc,&data[i],QUI) ;
    }

    while(true){
        
        xpthread_mutex_lock(&mutexWorkIndex,QUI) ;

        while((*data->WorkingIndex) < g->N){
            xpthread_cond_wait(&IndiceRaggiunto,&mutexWorkIndex,QUI) ;
        }

        if(error < eps || (*numiter) > maxiter){
            (*data->WorkingIndex) = -1 ;
            xpthread_mutex_unlock(&mutexWorkIndex,QUI) ;
            xpthread_cond_broadcast(&IndiceResettato,QUI) ;
            break ;
        }
        (*numiter) += 1 ;
        
        error = 0 ;
        S = 0 ;

        //copio i valori di NewX in X
        for(int i = 0 ; i < g->N ; i++){
            X[i] = NewX[i] ;
        }

        //calcolo contributi DeadNodes
        for(int i = 0 ; i < NumberOfDeadNodes ; i++){
                S += X[DeadNodes[i]] ;
        }
            
        S = (d/g->N) * S ;
            
        //calcolo vettore Y 
        for(int i = 0 ; i < NumberdOfNotDeadNodes ; i++){
                Y[NotDeadNodes[i]] = X[NotDeadNodes[i]]/g->out[NotDeadNodes[i]] ;
        }
        
        (*data->WorkingIndex) = 0 ;
    
        xpthread_mutex_unlock(&mutexWorkIndex,QUI) ;
        xpthread_cond_broadcast(&IndiceResettato,QUI) ;
        
    }
    
    (*data->WorkingIndex) = -1 ;

    //aspetto la fine dei thread ausiliari
    for(int i = 0 ; i < taux ; i++){
        xpthread_join(th[i],NULL,QUI) ;
    }

    free(X) ;
    free(Y) ;
    free(NotDeadNodes) ;
    free(DeadNodes) ;
    xpthread_mutex_destroy(&mutexWorkIndex,QUI) ;
    xpthread_cond_destroy(&IndiceRaggiunto,QUI) ;
    xpthread_cond_destroy(&IndiceResettato,QUI) ;
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
        
        xpthread_mutex_lock(data->mutex,QUI) ;
        
        while((*data->WorkingIndex) >= data->g->N){
            xpthread_cond_signal(data->IndiceRaggiunto,QUI) ;
            xpthread_cond_wait(data->IndiceResettato,data->mutex,QUI) ;
        }

        j = (*data->WorkingIndex) ;

        if(j == -1){ 
            xpthread_mutex_unlock(data->mutex,QUI) ;
            pthread_exit(NULL) ;
        }

        (*data->WorkingIndex) += 1 ;
        xpthread_mutex_unlock(data->mutex,QUI) ;
        
        
        //calcolo del nuovo fattore pagerank
        for(int i = 0 ; i < data->g->in[j].len ; i++){
            somma += data->Y[data->g->in[j].inArrow[i]] ;
        }

        newx = data->TeleFactor + (*data->S) + data->d * somma ;
        newe = fabs(data->X[j] -newx ) ;
        
        //caricamento del nuovo pagerank 
        data->NewX[j] = newx ;
        (*data->e ) += newe ;
        
    }
}

int comparazione_decrescente(const void *a, const void *b) {
    const TopElement *elem_a = (const TopElement *)a;
    const TopElement *elem_b = (const TopElement *)b;

    if (elem_a->value < elem_b->value) {
        return 1; 
    } else if (elem_a->value > elem_b->value) {
        return -1; 
    } else {
        return 0;
    }
}