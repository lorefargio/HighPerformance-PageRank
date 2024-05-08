#include "xerrori.h"
#define QUI __LINE__,__FILE__

void ParsingCommandLine(int *NumberOfTopNodes, int *MaxIteration, float *DampFactor, long *MaxError, int argc, char*argv[]) ;

int main(int argc, char *argv[]){
    int NumberOfTopNodes = 3 ;
    int MaxIteration = 100 ;
    float DampFactor = 0.9 ;
    long MaxError = 0 ;
    
    ParsingCommandLine(&NumberOfTopNodes,&MaxIteration,&DampFactor,&MaxError,argc,argv) ;

    printf("k : %d , m : %d , d : %f , e : %ld ,test:%s\n",NumberOfTopNodes,MaxIteration,DampFactor,MaxError,argv[argc-1]) ;
    return 0 ;
}

void ParsingCommandLine(int *NumberOfTopNodes, int *MaxIteration, float *DampFactor, long *MaxError, int argc, char*argv[]){
    int option ;

    while((option = getopt(argc,argv,"k:d:m:e:")) != -1){
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
                    (*MaxError) = strtol(optarg,NULL,10) ;
                }else{
                    termina("Errore nel parsing MaxError") ;
                }
            break ;

        }
    }
}