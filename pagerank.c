#include "xerrori.h"
#define QUI __LINE__,__FILE__
#define BufSize 10 

/*per ottimizzare la ricerca di archi entranti in uno specifico nodo 
gli array presenti nel tipo inmap andranno mantenuti sempre ordinati in modo crescente 
in modo da controllare efficentemente se un arco sia gia stato inserito o meno */
typedef struct {
    int *inArrow ;
} inmap ;

typedef struct {
    int N  ; //numero nodi grafo
    int *out ; //array con numero di archi uscenti da ogni nodo
    inmap *in ; //array con gli insiemi si archi entranti in ogni nodo
} grafo ;

typedef struct {
    int i ; //nodo di partenza dell'arco
    int j ; //nodo di arrivo dell'arco
} coppia ;

typedef struct {
    coppia *Buffer ;
    sem_t *FreePlace ;
    sem_t *ItemNumber ;
    int *buffindex ;
} dati ;

void ParsingCommandLine(int *NumberOfTopNodes, int *MaxIteration, float *DampFactor, double *MaxError,int *ThreadNumber ,char **FileName,int argc, char*argv[]) ;

void ReadingFile(char *FileName, coppia *B, int *indexP, sem_t *FreePlace, sem_t *ItemNumber ) ;

void *ArchManagement() ;

int main(int argc, char *argv[]){
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
    coppia buffer[BufSize] ;
    int indexP = 0 , indexC = 0 ;

    //inizzializzazione semafori per la gestione del buffer
    xsem_init(&FreePlace,0,BufSize,QUI) ;
    xsem_init(&ItemNumber,0,0,QUI) ;

    //inizzializzazione struttura dati da passare ai thread
    dati d[ThreadNumber] ;

    if(argc < 2){
        printf("Il nome del file è un parametro obbligatorio\n") ;
        printf("Uso: %s nomefile \n",argv[0]) ;
        return 1 ;
    }
    //parsing della linea di comando
    ParsingCommandLine(&NumberOfTopNodes,&MaxIteration,&DampFactor,&MaxError,&ThreadNumber,&FileName,argc,argv) ;
    
    //lettura del file e caricamento del buffer
    ReadingFile(FileName,&buffer,&indexP,&FreePlace,&ItemNumber) ;

    //printf("k : %d , m : %d , d : %f , e : %.7f , t: %d , file : %s\n",NumberOfTopNodes,MaxIteration,DampFactor,MaxError,ThreadNumber,FileName) ;
    return 0 ;
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

void ReadingFile(char *FileName, coppia *B, int *indexP, sem_t *FreePlace, sem_t *ItemNumber ){
    
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

    while((nread = getline(&line,&len,f)) != -1) {
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
                printf("r : %d , c : %d , n : %d\n",r,c,n) ;
            }else{
                i = atoi(&line[0]) ;
                j = atoi(&line[1]) ;

                //controlli sul valore dei nodi
                assert(1 <= i) ;
                assert(i <= r) ;

                assert(1 <= j) ;
                assert(j <= c) ;

                //inserimento dei nodi all'interno del buffer ;

                xsem_wait(FreePlace,QUI) ;
                B[*(indexP) % BufSize].i = i ;
                B[*(indexP) % BufSize].j = j ;
                printf("Scrittura dei valori nel buffer [%d] , i : %d , j : %d\n",(*indexP),i,j) ;
                (*indexP) += 1 ;
                xsem_post(ItemNumber,QUI) ;
            }
       }
    }

    free(line) ;
    fclose(f) ;
    
}
