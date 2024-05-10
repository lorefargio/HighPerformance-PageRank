#include "xerrori.h"
#define QUI __LINE__,__FILE__
#define BufSize 20 

/*per ottimizzare la ricerca di archi entranti in uno specifico nodo 
gli array presenti nel tipo inmap andranno mantenuti sempre ordinati in modo crescente 
in modo da controllare efficentemente se un arco sia gia stato inserito o meno */
typedef struct {
    int *inArrow ;
    int len ;
} inmap ;

typedef struct {
    int N  ; //numero nodi grafo
    int *out ; //array con numero di archi uscenti da ogni nodo
    inmap *in ; //array con gli insiemi di archi entranti in ogni nodo
    pthread_mutex_t *mutex_arr;
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
} dati ;

//Funzione che cernca un particolare intero all'interno di un array 
bool BinarySearch(int target, int *arr, int len) ;

//Funzione MergeSort
void merge(int *arr, int left, int mid, int right);
void mergeSort(int *arr, int left, int right) ;

//funzione che inizzializza i parametri inseriti dall'utente sulla linea di comando
void ParsingCommandLine(int *NumberOfTopNodes, int *MaxIteration, float *DampFactor, double *MaxError,int *ThreadNumber ,char **FileName,int argc, char*argv[]) ;

//funzione che ritorna il numero di nodi del grafo
int ReadingNumberOfNode(char *FileName) ;

//funzione che legge dal file gli archi e li inserisce sul buffer
void ReadingFile(char *FileName, void *arg ) ;

//thread body che gestisce gli archi
void *ArchManagement(void *arg) ;

int main(int argc, char *argv[]){
    if(argc < 2){
        printf("Il nome del file è un parametro obbligatorio\n") ;
        printf("Uso: %s nomefile \n",argv[0]) ;
        return 1 ;
    }
    
    //valore standrd dati in input
    int NumberOfTopNodes = 3 ;
    int MaxIteration = 100 ;
    float DampFactor = 0.9 ;
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
    printf("Node num %d \n",NodeNumber);
    grafo g ;
    g.N = NodeNumber ;
    g.out = calloc(NodeNumber,sizeof(int)) ; 
    g.mutex_arr = &mutexArr ;

    inmap *arr = malloc(NodeNumber*sizeof(inmap));

    //inizzializzazioni campi array di tipo inmap
    for(int i = 0 ; i < NodeNumber ; i++){
        arr[i].len = 0 ;
        arr[i].inArrow = malloc((arr[i].len)*sizeof(int)) ;
    }

    g.in = arr ;

    //inizzializzazione struttura dati per thread produttore
    dati produttore ;
    produttore.Buffer = buffer ;
    produttore.buffindex = &indexP ;
    produttore.FreePlace = &FreePlace ;
    produttore.ItemNumber = &ItemNumber ;

    //inizzializzazione struttura dati pre thread consumatore
    dati consumatore[ThreadNumber] ;
    
    //inizzializzazione struttura dati 
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
    
    //comunico ai consumatori che possono terminare 
    for(int i = 0 ; i < ThreadNumber ; i++){
        xsem_wait(&FreePlace,QUI) ;
        buffer[indexP % BufSize].i = -1 ; 
        buffer[indexP % BufSize].j = -1 ; 
        indexP++ ;
        xsem_post(&ItemNumber,QUI) ;
    }

    //attendo la fine dei consumatori
    for(int i = 0 ; i < ThreadNumber ; i++){
        xpthread_join(th[i],NULL,QUI) ;
    }

    printf("Lettura del file e inizzializzazione grafo terminata \n") ;
    
    
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

void ParsingCommandLine(int *NumberOfTopNodes, int *MaxIteration, float *DampFactor, double *MaxError, int *ThreadNumber , char **FileName, int argc, char*argv[]){
    int option ;

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

    char *line ;
    size_t len = 0 ;
    int nread ;

    if(f == NULL){
        termina("Errore apertura file") ;
    }

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
    dati *d = (dati *)arg ;

    FILE *f  = fopen(FileName,"r");
    //dati che rappresentano il numero di righe, colonne della matrice di adiacienza e il numero totale di archi 
    int r, c, n ;
    bool DatiIniziali = true ;

    //dati che rappresentano nodo entrante e uscente di un arco
    int i, j ;

    char *line ;
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
                //printf("Scrittura dei valori nel buffer [%d] , i : %d , j : %d\n",(*d->buffindex),i,j) ;
                (*d->buffindex) += 1 ;
                xsem_post(d->ItemNumber,QUI) ;
            }
       }
    }
    /*
    //inserisco un -1 all'interno del buffer per segnalare la fine della lettura 
    xsem_wait(d->FreePlace,QUI) ;
    d->Buffer[*(d->buffindex) % BufSize].i = -1 ;
    d->Buffer[*(d->buffindex) % BufSize].j = -1 ;
    printf("Inserisco -1 all'indice %d\n",*(d->buffindex) % BufSize) ;
    //(*d->buffindex) += 1 ;
    xsem_post(d->ItemNumber,QUI) ;
    */
    free(line) ;
    fclose(f) ;
    
}

void *ArchManagement(void *arg){
    dati *d = (dati *)arg ;
    int i , j ;

    while(true){
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
        
        //essendo il vettore in ordinato faccio una ricerca binaria per vedere se un valore è presente al suo interno
        
        xpthread_mutex_lock(d->g->mutex_arr,QUI) ;
        
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